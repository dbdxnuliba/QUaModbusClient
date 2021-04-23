#include "quamodbusdatablock.h"
#include "quamodbusclient.h"
#include "quamodbusvalue.h"

#ifdef QUA_ACCESS_CONTROL
#include <QUaPermissions>
#endif // QUA_ACCESS_CONTROL

quint32 QUaModbusDataBlock::m_minSamplingTime = 50;

QUaModbusDataBlock::QUaModbusDataBlock(QUaServer *server)
#ifndef QUA_ACCESS_CONTROL
	: QUaBaseObject(server)
#else
	: QUaBaseObjectProtected(server)
#endif // !QUA_ACCESS_CONTROL
{
	m_loopHandle = -1;
	m_firstSample = true;
	m_replyRead  = nullptr;
	m_type = nullptr;
	m_address = nullptr;
	m_size = nullptr;
	m_samplingTime = nullptr;
	m_data = nullptr;
	m_lastError = nullptr;
	m_values = nullptr;
	// NOTE : QObject parent might not be yet available in constructor
	type   ()->setDataTypeEnum(QMetaEnum::fromType<QModbusDataBlockType>());
	type   ()->setValue(QModbusDataBlockType::Invalid);
	address()->setDataType(QMetaType::Int);
	address()->setValue(-1);
	size   ()->setDataType(QMetaType::UInt);
	size   ()->setValue(0);
	samplingTime()->setDataType(QMetaType::UInt);
	samplingTime()->setValue(1000);
	lastError   ()->setDataTypeEnum(QMetaEnum::fromType<QModbusError>());
	lastError   ()->setValue(QModbusError::NoError);
	// set initial conditions
	type()        ->setWriteAccess(true);
	address()     ->setWriteAccess(true);
	size()        ->setWriteAccess(true);
	samplingTime()->setWriteAccess(true);
	data()        ->setMinimumSamplingInterval(1000);
	// handle state changes
	QObject::connect(type()        , &QUaBaseVariable::valueChanged, this, &QUaModbusDataBlock::on_typeChanged        , Qt::QueuedConnection);
	QObject::connect(address()     , &QUaBaseVariable::valueChanged, this, &QUaModbusDataBlock::on_addressChanged     , Qt::QueuedConnection);
	QObject::connect(size()        , &QUaBaseVariable::valueChanged, this, &QUaModbusDataBlock::on_sizeChanged        , Qt::QueuedConnection);
	QObject::connect(samplingTime(), &QUaBaseVariable::valueChanged, this, &QUaModbusDataBlock::on_samplingTimeChanged, Qt::QueuedConnection);
	QObject::connect(data()        , &QUaBaseVariable::valueChanged, this, &QUaModbusDataBlock::on_dataChanged        , Qt::QueuedConnection);
	// to safely update error in ua server thread
	QObject::connect(this, &QUaModbusDataBlock::updateLastError, this, &QUaModbusDataBlock::on_updateLastError);
	// set descriptions
	/*
	type        ()->setDescription(tr("Type of Modbus register for this block."));
	address     ()->setDescription(tr("Start register address for this block (with respect to the register type)."));
	size        ()->setDescription(tr("Size (in registers) for this block."));
	samplingTime()->setDescription(tr("Polling time (cycle time) to read this block."));
	data        ()->setDescription(tr("The current block values as per the last successfull read."));
	lastError   ()->setDescription(tr("The last error reported while reading or writing this block."));
	values      ()->setDescription(tr("List of converted values."));
	*/
}

QUaModbusDataBlock::~QUaModbusDataBlock()
{
	emit this->aboutToDestroy();
	emit m_values->aboutToClear();
	// stop loop
	if (m_loopHandle > 0)
	{
		this->client()->m_workerThread.stopLoopInThread(m_loopHandle);
	}	
	// make handle invalid **after** stopping loop in thread
	m_loopHandle = -1;
	// delete while block still valid, because in views values reference parent block
	for (auto value : m_values->values())
	{
		delete value;
	}
}

QUaProperty * QUaModbusDataBlock::type()
{
	if (!m_type)
	{
		m_type = this->browseChild<QUaProperty>("Type");
	}
	return m_type;
}

QUaProperty * QUaModbusDataBlock::address()
{
	if (!m_address)
	{
		m_address = this->browseChild<QUaProperty>("Address");
	}
	return m_address;
}

QUaProperty * QUaModbusDataBlock::size()
{
	if (!m_size)
	{
		m_size = this->browseChild<QUaProperty>("Size");
	}
	return m_size;
}

QUaProperty * QUaModbusDataBlock::samplingTime()
{
	if (!m_samplingTime)
	{
		m_samplingTime = this->browseChild<QUaProperty>("SamplingTime");
	}
	return m_samplingTime;
}

QUaBaseDataVariable * QUaModbusDataBlock::data()
{
	if (!m_data)
	{
		m_data = this->browseChild<QUaBaseDataVariable>("Data");
	}
	return m_data;
}

QUaBaseDataVariable * QUaModbusDataBlock::lastError()
{
	if (!m_lastError)
	{
		m_lastError = this->browseChild<QUaBaseDataVariable>("LastError");
	}
	return m_lastError;
}

QUaModbusValueList * QUaModbusDataBlock::values()
{
	if (!m_values)
	{
		m_values = this->browseChild<QUaModbusValueList>("Values");
	}
	return m_values;
}

void QUaModbusDataBlock::remove()
{
	// stop loop
	this->client()->m_workerThread.stopLoopInThread(m_loopHandle);
	// make handle invalid **after** stopping loop in thread
	m_loopHandle = -1;
	// call deleteLater in thread, so thread has time to stop loop first
	// NOTE : deleteLater will delete the object in the correct thread anyways
	this->client()->m_workerThread.execInThread([this]() {
		// then delete
		this->deleteLater();	
	}, Qt::EventPriority::LowEventPriority);
}

void QUaModbusDataBlock::on_typeChanged(const QVariant &value, const bool& networkChange)
{
	if (!networkChange)
	{
		return;
	}
	auto type = value.value<QModbusDataBlockType>();
	// set in thread for safety
	this->client()->m_workerThread.execInThread([this, type]() {
		m_registerType = static_cast<QModbusDataBlockType>(type);
	});
	// set data writable according to type
	if (type == QModbusDataBlockType::Coils ||
		type == QModbusDataBlockType::HoldingRegisters)
	{
		data()->setWriteAccess(true);
	}
	else
	{
		data()->setWriteAccess(false);
	}
	// emit
	emit this->typeChanged(type);
	// update permissions in values
	auto values = this->values()->values();
	for (auto value : values)
	{
		if (type == QUaModbusDataBlock::RegisterType::Coils ||
			type == QUaModbusDataBlock::RegisterType::HoldingRegisters)
		{
			value->value()->setWriteAccess(true);
		}
		else
		{
			value->value()->setWriteAccess(false);
		}
	}
}

void QUaModbusDataBlock::on_addressChanged(const QVariant & value, const bool& networkChange)
{
	if (!networkChange)
	{
		return;
	}
	auto address = value.value<int>();
	// set in thread for safety
	this->client()->m_workerThread.execInThread([this, address]() {
		m_startAddress = address;
	});
	// emit
	emit this->addressChanged(address);
}

void QUaModbusDataBlock::on_sizeChanged(const QVariant & value, const bool& networkChange)
{
	if (!networkChange)
	{
		return;
	}
	auto size = value.value<quint32>();
	// set in thread for safety
	this->client()->m_workerThread.execInThread([this, size]() {
		m_valueCount = size;
	});
	// emit
	emit this->sizeChanged(size);
}

void QUaModbusDataBlock::on_samplingTimeChanged(const QVariant & value, const bool& networkChange)
{
	if (!networkChange)
	{
		return;
	}
	// check minimum sampling time
	auto samplingTime = value.value<quint32>();
	// do not allow less than minimum
	if (samplingTime < QUaModbusDataBlock::m_minSamplingTime)
	{
		// set minumum
		this->samplingTime()->setValue(QUaModbusDataBlock::m_minSamplingTime);
		// the previous will trigger the event again
		// emit
		emit this->samplingTimeChanged(QUaModbusDataBlock::m_minSamplingTime);
		return;
	}
	// stop old loop
	this->client()->m_workerThread.stopLoopInThread(m_loopHandle);
	// make handle invalid **after** stopping loop in thread
	m_loopHandle = -1;
	// start new loop
	this->startLoop();
	// update ua sample interval for data
	this->data()->setMinimumSamplingInterval((double)samplingTime);
	// emit
	emit this->samplingTimeChanged(samplingTime);
}

void QUaModbusDataBlock::on_dataChanged(const QVariant & value, const bool& networkChange)
{
	if (!networkChange)
	{
		return;
	}
	// convert data
	QVector<quint16> data = QUaModbusDataBlock::variantToInt16Vect(value);
	// emit
	emit this->dataChanged(data);
	// write to modbus
	this->setModbusData(data);
}

void QUaModbusDataBlock::on_updateLastError(const QModbusError & error)
{
	// avoid update or emit if no change, improves performance
	if (error == this->getLastError())
	{
		return;
	}
	this->lastError()->setValue(error);
	// NOTE : need to add custom signal because OPC UA valueChanged
	//        only works for changes through network
	// emit
	emit this->lastErrorChanged(error);
	// update errors in values
	auto values = this->values()->values();
	for (auto value : values)
	{
		// need to keep configuration error if value is still not well configured
		auto isWellConfigured = value->isWellConfigured();
		if (!isWellConfigured)
		{
			continue;
		}
		value->setLastError(error);
	}
}

QUaModbusDataBlockList * QUaModbusDataBlock::list() const
{
	return qobject_cast<QUaModbusDataBlockList*>(this->parent());
}

QUaModbusClient * QUaModbusDataBlock::client() const
{
	return this->list()->client();
}

void QUaModbusDataBlock::startLoop()
{
	auto samplingTime = this->samplingTime()->value().value<quint32>();
	// exec read request in client thread
	m_loopHandle = this->client()->m_workerThread.startLoopInThread(
	[this]() {
		//Q_ASSERT(m_loopHandle > 0); // NOTE : this does happen when cleaning all blocks form a client
		if (m_loopHandle <= 0)
		{
			return;
		}
		auto client = this->client();
		// TODO : can happen in shutdown? possible BUG
		if (!client)
		{
			return;
		}
		// check if ongoing request
		if (m_replyRead)
		{
			return;
		}
		// check if request is valid
		if (m_registerType == QModbusDataBlockType::Invalid)
		{
			emit this->updateLastError(QModbusError::ConfigurationError);
			return;
		}
		if (m_startAddress < 0)
		{
			emit this->updateLastError(QModbusError::ConfigurationError);
			return;
		}
		if (m_valueCount == 0)
		{
			emit this->updateLastError(QModbusError::ConfigurationError);
			return;
		}
		// check if connected
		auto state = client->getState();
		if (state != QModbusState::ConnectedState)
		{
			if (!m_firstSample)
			{
				// force update last modbus value
				auto values = this->values()->values();
				for (auto value : values)
				{
					emit value->valueChanged(value->getValue());
				}
				m_firstSample = true;
			}
			auto clientError = client->getLastError();
			emit this->updateLastError(clientError);
			return;
		}
		// create and send request		
		auto serverAddress = client->getServerAddress();
		// NOTE : need to pass in a fresh QModbusDataUnit instance or reply for coils returns empty
		//        wierdly, registers work fine when passing m_modbusDataUnit
		m_replyRead = client->m_modbusClient->sendReadRequest(
			QModbusDataUnit(
				static_cast<QModbusDataUnit::RegisterType>(m_registerType),
				m_startAddress, 
				m_valueCount
			)
			, serverAddress
		);
		// check if no error
		if (!m_replyRead)
		{
			if (!client->m_disconnectRequested)
			{
				emit this->updateLastError(QModbusError::ReplyAbortedError);
			}
			return;
		}
		// check if finished immediately (ignore)
		if (m_replyRead->isFinished())
		{
			// broadcast replies return immediately
			m_replyRead->deleteLater();
			m_replyRead = nullptr;
			return;
		}
		// subscribe to finished
		QObject::connect(m_replyRead, &QModbusReply::finished, this,
			[this]() {
				// NOTE : exec'd in ua server thread (not in worker thread)
				auto client = this->client();
				Q_CHECK_PTR(client);
				if (client->m_disconnectRequested || client->getState() != QModbusState::ConnectedState)
				{
					m_replyRead = nullptr;
					this->setLastError(QModbusError::ReplyAbortedError);
					return;
				}
				// check if reply still valid
				if (!m_replyRead)
				{
					this->setLastError(QModbusError::ReplyAbortedError);
					return;
				}
				// handle error
				auto error = m_replyRead->error();
				this->setLastError(error);
				// update block value
				QVector<quint16> data = m_replyRead->result().values();
				// TODO : early exit when refactor QUaModbusValue::setValue
				if (error == QModbusError::NoError)
				{
					Q_ASSERT(data.count() == m_valueCount);
					this->setData(data, false);
				}
				// update modbus values and errors
				auto values = this->values()->values();
				for (auto value : values)
				{
					value->setValue(data, error, m_firstSample);
				}
				// delete reply on next event loop exec
				m_replyRead->deleteLater();
				m_replyRead = nullptr;
				m_firstSample = false;
			}, Qt::QueuedConnection);
		}, samplingTime);
	Q_ASSERT(m_loopHandle > 0);
}

bool QUaModbusDataBlock::loopRunning()
{
	return m_loopHandle >= 0;
}

void QUaModbusDataBlock::setModbusData(const QVector<quint16>& data)
{
	// exec write request in client thread
	this->client()->m_workerThread.execInThread(
	[this, data]() {
		auto client = this->client();
		// check if request is valid
		if (m_registerType != QModbusDataBlockType::Coils &&
			m_registerType != QModbusDataBlockType::HoldingRegisters)
		{
			return;
		}
		if (m_startAddress < 0)
		{
			emit this->updateLastError(QModbusError::ConfigurationError);
			return;
		}
		if (m_valueCount == 0)
		{
			emit this->updateLastError(QModbusError::ConfigurationError);
			return;
		}
		// check if connected
		auto state = client->getState();
		if (state != QModbusState::ConnectedState)
		{
			auto clientError = client->getLastError();
			emit this->updateLastError(clientError);
			return;
		}
		// create data target 
		QModbusDataUnit dataToWrite(
			static_cast<QModbusDataUnit::RegisterType>(m_registerType), 
			m_startAddress, 
			data
		);
		// create and send request
		auto serverAddress = client->getServerAddress();
		QModbusReply * p_reply = client->m_modbusClient->sendWriteRequest(dataToWrite, serverAddress);
		if (!p_reply)
		{
			emit this->updateLastError(QModbusError::ReplyAbortedError);
			return;
		}
		// subscribe to finished
		QObject::connect(p_reply, &QModbusReply::finished, this, 
		[this, p_reply]() mutable {
			// NOTE : exec'd in ua server thread (not in worker thread)
			// check if reply still valid
			if (!p_reply)
			{
				auto error = QModbusError::ReplyAbortedError;
				this->setLastError(error);
				return;
			}
			// handle error
			auto error = p_reply->error();
			this->setLastError(error);
			// delete reply on next event loop exec
			p_reply->deleteLater();
			p_reply = nullptr;
		}, Qt::QueuedConnection);
	});
}

QDomElement QUaModbusDataBlock::toDomElement(QDomDocument & domDoc) const
{
	// add block element
	QDomElement elemBlock = domDoc.createElement(QUaModbusDataBlock::staticMetaObject.className());
#ifdef QUA_ACCESS_CONTROL
	// set parmissions if any
	if (this->hasPermissionsObject())
	{
		elemBlock.setAttribute("Permissions", this->permissionsObject()->nodeId());
	}
#endif // QUA_ACCESS_CONTROL
	// set block attributes
	elemBlock.setAttribute("BrowseName"  , this->browseName().name());
	elemBlock.setAttribute("Type"        , QMetaEnum::fromType<QModbusDataBlockType>().valueToKey(getType()));
	elemBlock.setAttribute("Address"     , getAddress());
	elemBlock.setAttribute("Size"        , getSize());
	elemBlock.setAttribute("SamplingTime", getSamplingTime());
	// add value list element
	auto elemValueList = const_cast<QUaModbusDataBlock*>(this)->values()->toDomElement(domDoc);
	elemBlock.appendChild(elemValueList);
	// return block element
	return elemBlock;
}

void QUaModbusDataBlock::fromDomElement(QDomElement & domElem, QQueue<QUaLog>& errorLogs)
{
	// get client attributes (BrowseName must be already set)
	QString strBrowseName = domElem.attribute("BrowseName", "");
	Q_ASSERT(this->browseName() == QUaQualifiedName(strBrowseName));
#ifdef QUA_ACCESS_CONTROL
	// load permissions if any
	if (domElem.hasAttribute("Permissions") && !domElem.attribute("Permissions").isEmpty())
	{
		QString strError = this->setPermissions(domElem.attribute("Permissions"));
		if (strError.contains("Error"))
		{
			errorLogs << QUaLog(
				strError,
				QUaLogLevel::Error,
				QUaLogCategory::Serialization
			);
		}
	}
#endif // QUA_ACCESS_CONTROL
	bool bOK;
	// Type
	auto type = QMetaEnum::fromType<QModbusDataBlockType>().keysToValue(domElem.attribute("Type").toUtf8(), &bOK);
	if (bOK)
	{
		this->type()->setValue(type);
		// NOTE : force internal update
		this->on_typeChanged(type, true);
	}
	else
	{
		errorLogs << QUaLog(
			tr("Invalid Type attribute '%1' in Block %2. Default value set.").arg(type).arg(strBrowseName),
			QUaLogLevel::Warning,
			QUaLogCategory::Serialization
		);
	}
	
	// Address
	auto address = domElem.attribute("Address").toInt(&bOK);
	if (bOK)
	{
		this->setAddress(address);
	}
	else
	{
		errorLogs << QUaLog(
			tr("Invalid Address attribute '%1' in Block %2. Default value set.").arg(address).arg(strBrowseName),
			QUaLogLevel::Warning,
			QUaLogCategory::Serialization
		);
	}
	// Size
	auto size = domElem.attribute("Size").toUInt(&bOK);
	if (bOK)
	{
		this->setSize(size);
	}
	else
	{
		errorLogs << QUaLog(
			tr("Invalid Size attribute '%1' in Block %2. Default value set.").arg(size).arg(strBrowseName),
			QUaLogLevel::Warning,
			QUaLogCategory::Serialization
		);
	}
	// SamplingTime
	auto samplingTime = domElem.attribute("SamplingTime").toUInt(&bOK);
	if (bOK)
	{
		this->setSamplingTime(samplingTime);
	}
	else
	{
		errorLogs << QUaLog(
			tr("Invalid SamplingTime attribute '%1' in Block %2. Default value set.").arg(samplingTime).arg(strBrowseName),
			QUaLogLevel::Warning,
			QUaLogCategory::Serialization
		);
	}
	// get value list
	QDomElement elemValueList = domElem.firstChildElement(QUaModbusValueList::staticMetaObject.className());
	if (!elemValueList.isNull())
	{
		values()->fromDomElement(elemValueList, errorLogs);
	}
	else
	{
		errorLogs << QUaLog(
			tr("Block %1 does not have a QUaModbusValueList child. No values will be loaded.").arg(strBrowseName),
			QUaLogLevel::Warning,
			QUaLogCategory::Serialization
		);
	}
}

QVector<quint16> QUaModbusDataBlock::variantToInt16Vect(const QVariant & value)
{
	QVector<quint16> data;
	// check if valid
	if (!value.isValid())
	{
		return data;
	}
	// convert
	QSequentialIterable iterable = value.value<QSequentialIterable>();
	QSequentialIterable::const_iterator it = iterable.begin();
	const QSequentialIterable::const_iterator end = iterable.end();
	for (; it != end; ++it) {
		data << (*it).value<quint16>();
	}
	return data;
}

QModbusDataBlockType QUaModbusDataBlock::getType() const
{
	return const_cast<QUaModbusDataBlock*>(this)->type()->value().value<QModbusDataBlockType>();
}

void QUaModbusDataBlock::setType(const QModbusDataBlockType & type)
{
	this->type()->setValue(type);
	this->on_typeChanged(type, true);
}

int QUaModbusDataBlock::getAddress() const
{
	return const_cast<QUaModbusDataBlock*>(this)->address()->value().toInt();
}

void QUaModbusDataBlock::setAddress(const int & address)
{
	this->address()->setValue(address);
	this->on_addressChanged(address, true);
}

quint32 QUaModbusDataBlock::getSize() const
{
	return const_cast<QUaModbusDataBlock*>(this)->size()->value().value<quint32>();
}

void QUaModbusDataBlock::setSize(const quint32 & size)
{
	this->size()->setValue(size);
	this->on_sizeChanged(size, true);
}

quint32 QUaModbusDataBlock::getSamplingTime() const
{
	return const_cast<QUaModbusDataBlock*>(this)->samplingTime()->value().value<quint32>();
}

void QUaModbusDataBlock::setSamplingTime(const quint32 & samplingTime)
{
	this->samplingTime()->setValue(samplingTime);
	this->on_samplingTimeChanged(samplingTime, true);
}

QVector<quint16> QUaModbusDataBlock::getData() const
{
	return QUaModbusDataBlock::variantToInt16Vect(const_cast<QUaModbusDataBlock*>(this)->data()->value());
}

void QUaModbusDataBlock::setData(const QVector<quint16>& data, const bool &writeModbus/* = true*/)
{
	Q_ASSERT_X(data.count() == this->getSize(), "QUaModbusDataBlock::setData", "Received block of incorrect size");
	auto varData = QVariant::fromValue(data);
	// set on OPC
	this->data()->setValue(varData); // TODO : check of memory leak when writing array
	// emit change c++
	emit this->dataChanged(data);
	// check if write to modbus
	if (!writeModbus)
	{
		return;
	}
	this->setModbusData(data);
}

QModbusError QUaModbusDataBlock::getLastError() const
{
	return const_cast<QUaModbusDataBlock*>(this)->lastError()->value().value<QModbusError>();
}

void QUaModbusDataBlock::setLastError(const QModbusError & error)
{
	// call internal slot on_updateLastError
	emit this->updateLastError(error);
}

bool QUaModbusDataBlock::isWellConfigured() const
{
	if (
		m_registerType == QModbusDataBlockType::Invalid ||
		m_startAddress < 0 ||
		m_valueCount == 0
		)
	{
		return false;
	}
	return true;
}

