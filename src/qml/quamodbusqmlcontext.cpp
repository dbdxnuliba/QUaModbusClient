#include "quamodbusqmlcontext.h"

#ifdef QUA_ACCESS_CONTROL
#include <QUaUser>
#include <QUaPermissions>
#endif // QUA_ACCESS_CONTROL

/******************************************************************************************************
*/

QUaModbusValueQmlContext::QUaModbusValueQmlContext(QObject* parent) : QObject(parent)
{
	m_value = nullptr;
#ifdef QUA_ACCESS_CONTROL
	m_canRead  = true;
	m_canWrite = true;
	auto context = qobject_cast<QUaModbusDataBlockQmlContext*>(parent);
	Q_ASSERT(context);
	m_loggedUser = context->loggedUser();
#endif // QUA_ACCESS_CONTROL
}

QString QUaModbusValueQmlContext::valueId() const
{
	Q_ASSERT(m_value);
	// NOTE : no need for AC because id can be read by js object key anyways
//#ifdef QUA_ACCESS_CONTROL
//	if (!m_canRead)
//	{
//		return QString();
//	}
//#endif // QUA_ACCESS_CONTROL
	return m_value->browseName().name();
}

QModbusValueType QUaModbusValueQmlContext::type() const
{
	Q_ASSERT(m_value);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead)
	{
		return QModbusValueType::Invalid;
	}
#endif // QUA_ACCESS_CONTROL
	return m_value->getType();
}

void QUaModbusValueQmlContext::setType(const int& type)
{
	Q_ASSERT(m_value);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead || !m_canWrite)
	{
		return;
	}
#endif // QUA_ACCESS_CONTROL
	m_value->setType(static_cast<QModbusValueType>(type));
}

quint16 QUaModbusValueQmlContext::registersUsed() const
{
	Q_ASSERT(m_value);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead)
	{
		return 0;
	}
#endif // QUA_ACCESS_CONTROL
	return m_value->getRegistersUsed();
}

int QUaModbusValueQmlContext::addressOffset() const
{
	Q_ASSERT(m_value);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead)
	{
		return -1;
	}
#endif // QUA_ACCESS_CONTROL
	return m_value->getAddressOffset();
}

void QUaModbusValueQmlContext::setAddressOffset(const int& addressOffset)
{
	Q_ASSERT(m_value);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead || !m_canWrite)
	{
		return;
	}
#endif // QUA_ACCESS_CONTROL
	m_value->setAddressOffset(addressOffset);
}

QVariant QUaModbusValueQmlContext::value() const
{
	Q_ASSERT(m_value);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead)
	{
		return QVariant();
	}
#endif // QUA_ACCESS_CONTROL
	return m_value->getValue();
}

void QUaModbusValueQmlContext::setValue(const QVariant& value)
{
	Q_ASSERT(m_value);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead || !m_canWrite)
	{
		return;
	}
#endif // QUA_ACCESS_CONTROL
	m_value->setValue(value);
}

QModbusError QUaModbusValueQmlContext::lastError() const
{
	Q_ASSERT(m_value);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead)
	{
		return QModbusError::UnknownError;
	}
#endif // QUA_ACCESS_CONTROL
	return m_value->getLastError();
}

#ifdef QUA_ACCESS_CONTROL
bool QUaModbusValueQmlContext::canRead() const
{
	return m_canRead;
}

bool QUaModbusValueQmlContext::canWrite() const
{
	return m_canWrite;
}
#endif // QUA_ACCESS_CONTROL

void QUaModbusValueQmlContext::bindValue(QUaModbusValue* value)
{
	// check valid arg
	Q_ASSERT(value);
	if (!value) { return; }
	// copy reference
	m_value = value;
#ifdef QUA_ACCESS_CONTROL
	QObject::connect(m_value, &QUaModbusClient::permissionsObjectChanged, this,
		[this]() {
			this->updatePermissions();
		});
#endif // QUA_ACCESS_CONTROL
	// susbcribe to changes
	QObject::connect(m_value, &QUaModbusValue::typeChanged         , this, &QUaModbusValueQmlContext::typeChanged);
	QObject::connect(m_value, &QUaModbusValue::registersUsedChanged, this, &QUaModbusValueQmlContext::registersUsedChanged);
	QObject::connect(m_value, &QUaModbusValue::addressOffsetChanged, this, &QUaModbusValueQmlContext::addressOffsetChanged);
	QObject::connect(m_value, &QUaModbusValue::valueChanged        , this, &QUaModbusValueQmlContext::valueChanged);
	QObject::connect(m_value, &QUaModbusValue::lastErrorChanged    , this, &QUaModbusValueQmlContext::lastErrorChanged);
}

#ifdef QUA_ACCESS_CONTROL
void QUaModbusValueQmlContext::updateLoggedUser(QUaUser* loggedUser)
{
	// notify downstream
	m_loggedUser = loggedUser;
	emit this->userChanged();
	// update members
	this->updatePermissions();
}

void QUaModbusValueQmlContext::updatePermissions()
{
	auto perms = m_value->permissionsObject();
	m_canRead = !perms ? true : perms->canUserRead(m_loggedUser);
	emit this->canReadChanged();
	m_canWrite = !perms ? true : perms->canUserWrite(m_loggedUser);
	emit this->canWriteChanged();
}
#endif // QUA_ACCESS_CONTROL

/******************************************************************************************************
*/

QUaModbusDataBlockQmlContext::QUaModbusDataBlockQmlContext(QObject* parent) : QObject(parent)
{
	m_block = nullptr;
	// forward signal
	QObject::connect(this, &QUaModbusDataBlockQmlContext::valuesChanged, this, &QUaModbusDataBlockQmlContext::valuesModelChanged);
#ifdef QUA_ACCESS_CONTROL
	m_canRead  = true;
	m_canWrite = true;
	auto context = qobject_cast<QUaModbusClientQmlContext*>(parent);
	Q_ASSERT(context);
	m_loggedUser = context->loggedUser();
#endif // QUA_ACCESS_CONTROL
}

QString QUaModbusDataBlockQmlContext::blockId() const
{
	Q_ASSERT(m_block);
	// NOTE : no need for AC because id can be read by js object key anyways
//#ifdef QUA_ACCESS_CONTROL
//	if (!m_canRead)
//	{
//		return QString();
//	}
//#endif // QUA_ACCESS_CONTROL
	return m_block->browseName().name();
}

QModbusDataBlockType QUaModbusDataBlockQmlContext::type() const
{
	Q_ASSERT(m_block);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead)
	{
		return QModbusDataBlockType::Invalid;
	}
#endif // QUA_ACCESS_CONTROL
	return m_block->getType();
}

void QUaModbusDataBlockQmlContext::setType(const int& type)
{

	Q_ASSERT(m_block);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead || !m_canWrite)
	{
		return;
	}
#endif // QUA_ACCESS_CONTROL
	m_block->setType(static_cast<QModbusDataBlockType>(type));
}

int QUaModbusDataBlockQmlContext::address() const
{
	Q_ASSERT(m_block);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead)
	{
		return -1;
	}
#endif // QUA_ACCESS_CONTROL
	return m_block->getAddress();
}

void QUaModbusDataBlockQmlContext::setAddress(const int& address)
{

	Q_ASSERT(m_block);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead || !m_canWrite)
	{
		return;
	}
#endif // QUA_ACCESS_CONTROL
	m_block->setAddress(address);
}

quint32 QUaModbusDataBlockQmlContext::size() const
{
	Q_ASSERT(m_block);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead)
	{
		return 0;
	}
#endif // QUA_ACCESS_CONTROL
	return m_block->getSize();
}

void QUaModbusDataBlockQmlContext::setSize(const quint32& size)
{
	Q_ASSERT(m_block);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead || !m_canWrite)
	{
		return;
	}
#endif // QUA_ACCESS_CONTROL
	m_block->setSize(size);
}

quint32 QUaModbusDataBlockQmlContext::samplingTime() const
{
	Q_ASSERT(m_block);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead)
	{
		return 0;
	}
#endif // QUA_ACCESS_CONTROL
	return m_block->getSamplingTime();
}

void QUaModbusDataBlockQmlContext::setSamplingTime(const quint32& samplingTime)
{
	Q_ASSERT(m_block);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead || !m_canWrite)
	{
		return;
	}
#endif // QUA_ACCESS_CONTROL
	m_block->setSamplingTime(samplingTime);
}

QModbusError QUaModbusDataBlockQmlContext::lastError() const
{
	Q_ASSERT(m_block);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead)
	{
		return QModbusError::UnknownError;
	}
#endif // QUA_ACCESS_CONTROL
	return m_block->getLastError();
}

#ifdef QUA_ACCESS_CONTROL
bool QUaModbusDataBlockQmlContext::canRead() const
{
	return m_canRead;
}

bool QUaModbusDataBlockQmlContext::canWrite() const
{
	return m_canWrite;
}
#endif // QUA_ACCESS_CONTROL

QVariantMap QUaModbusDataBlockQmlContext::values()
{
	return m_values;
}

QVariant QUaModbusDataBlockQmlContext::valuesModel()
{
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead)
	{
		return QVariant();
	}
#endif // QUA_ACCESS_CONTROL
	QList<QObject*> retList;
	for (auto &valueVariant : m_values)
	{
		auto context = valueVariant.value<QUaModbusValueQmlContext*>();
#ifdef QUA_ACCESS_CONTROL
		if (!context->canRead())
		{
			continue;
		}
#endif // QUA_ACCESS_CONTROL
		retList << context;
	}
	return QVariant::fromValue(retList);
}

void QUaModbusDataBlockQmlContext::bindBlock(QUaModbusDataBlock* block)
{
	// check valid arg
	Q_ASSERT(block);
	if (!block) { return; }
	// copy reference
	m_block = block;
#ifdef QUA_ACCESS_CONTROL
	QObject::connect(m_block, &QUaModbusClient::permissionsObjectChanged, this,
		[this]() {
			this->updatePermissions();
		});
#endif // QUA_ACCESS_CONTROL
	// susbcribe to changes
	// QUaModbusDataBlock
	QObject::connect(m_block, &QUaModbusDataBlock::typeChanged        , this, &QUaModbusDataBlockQmlContext::typeChanged);
	QObject::connect(m_block, &QUaModbusDataBlock::addressChanged     , this, &QUaModbusDataBlockQmlContext::addressChanged);
	QObject::connect(m_block, &QUaModbusDataBlock::sizeChanged        , this, &QUaModbusDataBlockQmlContext::sizeChanged);
	QObject::connect(m_block, &QUaModbusDataBlock::samplingTimeChanged, this, &QUaModbusDataBlockQmlContext::samplingTimeChanged);
	//QObject::connect(m_block, &QUaModbusDataBlock::dataChanged, this, &QUaModbusDataBlockQmlContext::dataChanged);
	QObject::connect(m_block, &QUaModbusDataBlock::lastErrorChanged   , this, &QUaModbusDataBlockQmlContext::lastErrorChanged);
	// QUaModbusValueList
	this->bindValues(m_block->values());
}

void QUaModbusDataBlockQmlContext::clear()
{
	// delete children contexts
	while (!m_values.isEmpty())
	{
		auto context = m_values.take(m_values.firstKey()).value<QUaModbusValueQmlContext*>();
		// N/A : context->clear();
		delete context;
	}
	// clear block qml refs
	m_values.clear();
	emit this->valuesChanged();
}

#ifdef QUA_ACCESS_CONTROL
QUaUser* QUaModbusDataBlockQmlContext::loggedUser() const
{
	return m_loggedUser;
}

void QUaModbusDataBlockQmlContext::updateLoggedUser(QUaUser* loggedUser)
{
	// notify downstream
	m_loggedUser = loggedUser;
	emit this->userChanged();
	// update members
	this->updatePermissions();
}

void QUaModbusDataBlockQmlContext::updatePermissions()
{
	auto perms = m_block->permissionsObject();
	m_canRead = !perms ? true : perms->canUserRead(m_loggedUser);
	emit this->canReadChanged();
	m_canWrite = !perms ? true : perms->canUserWrite(m_loggedUser);
	emit this->canWriteChanged();
	// update models
	// NOTE : also triggers valuesModelChanged
	emit this->valuesChanged();
}
#endif // QUA_ACCESS_CONTROL

void QUaModbusDataBlockQmlContext::bindValues(QUaModbusValueList* values)
{
	// check valid arg
	Q_ASSERT(values);
	if (!values) { return; }
	// bind about to clear
	QObject::connect(values, &QUaModbusValueList::aboutToClear, this,
	[this]() {
		this->clear();
	});
	// bind existing
	for (auto value : values->values())
	{
		// bind existing
		this->bindValue(value);
	}
	// bind new added
	QObject::connect(values, &QUaNode::childAdded, this,
	[this](QUaNode* node) {
		// bind new
		auto value = qobject_cast<QUaModbusValue*>(node);
		Q_ASSERT(value);
		this->bindValue(value);
	}/*, Qt::QueuedConnection // NOTE : do not queue or will not be available on view load */);
}

void QUaModbusDataBlockQmlContext::bindValue(QUaModbusValue* value)
{
	// get id
	QString strId = value->browseName().name();
	Q_ASSERT(!strId.isEmpty() && !strId.isNull());
	// add param context to map
	auto context = new QUaModbusValueQmlContext(this);
	context->bindValue(value);
	Q_ASSERT(!m_values.contains(strId));
	m_values[strId] = QVariant::fromValue(context);
#ifdef QUA_ACCESS_CONTROL
	QObject::connect(this, &QUaModbusDataBlockQmlContext::userChanged, context, [this, context]() {
		context->updateLoggedUser(m_loggedUser);
	});
#endif // QUA_ACCESS_CONTROL
	// subscribe to destroyed
	QObject::connect(value, &QObject::destroyed, context,
	[this, value]() {
		this->removeValue(value);
	});
	// notify changes
	emit this->valuesChanged();
}

void QUaModbusDataBlockQmlContext::removeValue(QUaModbusValue* value)
{
	QString strId = value->browseName().name();
	Q_ASSERT(m_values.contains(strId));
	delete m_values.take(strId).value<QUaModbusValueQmlContext*>();
	// notify changes
	emit this->valuesChanged();
}

/******************************************************************************************************
*/

QUaModbusClientQmlContext::QUaModbusClientQmlContext(QObject* parent) : QObject(parent)
{
	// forward signal
	QObject::connect(this, &QUaModbusClientQmlContext::blocksChanged, this, &QUaModbusClientQmlContext::blocksModelChanged);
#ifdef QUA_ACCESS_CONTROL
	m_canRead  = true;
	m_canWrite = true;
	auto context = qobject_cast<QUaModbusQmlContext*>(parent);
	Q_ASSERT(context);
	m_loggedUser = context->loggedUser();
#endif // QUA_ACCESS_CONTROL
}

QString QUaModbusClientQmlContext::clientId() const
{
	Q_ASSERT(m_client);
	// NOTE : no need for AC because id can be read by js object key anyways
//#ifdef QUA_ACCESS_CONTROL
//	if (!m_canRead)
//	{
//		return QString();
//	}
//#endif // QUA_ACCESS_CONTROL
	return m_client->browseName().name();
}

QModbusClientType QUaModbusClientQmlContext::type() const
{
	Q_ASSERT(m_client);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead)
	{
		return QModbusClientType::Invalid;
	}
#endif // QUA_ACCESS_CONTROL
	return m_client->getType();
}

quint8 QUaModbusClientQmlContext::serverAddress() const
{
	Q_ASSERT(m_client);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead)
	{
		return 0;
	}
#endif // QUA_ACCESS_CONTROL
	return m_client->getServerAddress();
}

void QUaModbusClientQmlContext::setServerAddress(const quint8& serverAddress)
{
	Q_ASSERT(m_client);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead || !m_canWrite)
	{
		return;
	}
#endif // QUA_ACCESS_CONTROL
	m_client->setServerAddress(serverAddress);
}

bool QUaModbusClientQmlContext::keepConnecting() const
{
	Q_ASSERT(m_client);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead)
	{
		return false;
	}
#endif // QUA_ACCESS_CONTROL
	return m_client->getKeepConnecting();
}

void QUaModbusClientQmlContext::setKeepConnecting(const bool& keepConnecting)
{
	Q_ASSERT(m_client);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead || !m_canWrite)
	{
		return;
	}
#endif // QUA_ACCESS_CONTROL
	m_client->setKeepConnecting(keepConnecting);
}

QModbusState QUaModbusClientQmlContext::state() const
{
	Q_ASSERT(m_client);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead)
	{
		return QModbusState::UnconnectedState;
	}
#endif // QUA_ACCESS_CONTROL
	return m_client->getState();
}

QModbusError QUaModbusClientQmlContext::lastError() const
{
	Q_ASSERT(m_client);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead)
	{
		return QModbusError::UnknownError;
	}
#endif // QUA_ACCESS_CONTROL
	return m_client->getLastError();
}

#ifdef QUA_ACCESS_CONTROL
bool QUaModbusClientQmlContext::canRead() const
{
	return m_canRead;
}

bool QUaModbusClientQmlContext::canWrite() const
{
	return m_canWrite;
}
#endif // QUA_ACCESS_CONTROL

QString QUaModbusClientQmlContext::networkAddress() const
{
	Q_ASSERT(m_client);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead)
	{
		return QString();
	}
#endif // QUA_ACCESS_CONTROL
	auto tcp = qobject_cast<QUaModbusTcpClient*>(m_client);
	if (!tcp)
	{
		return QString();
	}
	return tcp->getNetworkAddress();
}

void QUaModbusClientQmlContext::setNetworkAddress(const QString& networkAddress)
{
	Q_ASSERT(m_client);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead || !m_canWrite)
	{
		return;
	}
#endif // QUA_ACCESS_CONTROL
	auto tcp = qobject_cast<QUaModbusTcpClient*>(m_client);
	if (!tcp)
	{
		return;
	}
	tcp->setNetworkAddress(networkAddress);
}

quint16 QUaModbusClientQmlContext::networkPort() const
{
	Q_ASSERT(m_client);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead)
	{
		return 0;
	}
#endif // QUA_ACCESS_CONTROL
	auto tcp = qobject_cast<QUaModbusTcpClient*>(m_client);
	if (!tcp)
	{
		return 0;
	}
	return tcp->getNetworkPort();
}

void QUaModbusClientQmlContext::setNetworkPort(const quint16& networkPort)
{
	Q_ASSERT(m_client);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead || !m_canWrite)
	{
		return;
	}
#endif // QUA_ACCESS_CONTROL
	auto tcp = qobject_cast<QUaModbusTcpClient*>(m_client);
	if (!tcp)
	{
		return;
	}
	tcp->setNetworkPort(networkPort);
}

QString QUaModbusClientQmlContext::comPort() const
{
	Q_ASSERT(m_client);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead)
	{
		return QString();
	}
#endif // QUA_ACCESS_CONTROL
	auto serial = qobject_cast<QUaModbusRtuSerialClient*>(m_client);
	if (!serial)
	{
		return QString();
	}
	return serial->getComPort();
}

void QUaModbusClientQmlContext::setComPort(const QString& strComPort)
{
	Q_ASSERT(m_client);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead || !m_canWrite)
	{
		return;
	}
#endif // QUA_ACCESS_CONTROL
	auto serial = qobject_cast<QUaModbusRtuSerialClient*>(m_client);
	if (!serial)
	{
		return;
	}
	return serial->setComPort(strComPort);
}

QParity QUaModbusClientQmlContext::parity() const
{
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead)
	{
		return QParity::UnknownParity;
	}
#endif // QUA_ACCESS_CONTROL
	Q_ASSERT(m_client);
	auto serial = qobject_cast<QUaModbusRtuSerialClient*>(m_client);
	if (!serial)
	{
		return QParity::UnknownParity;
	}
	return serial->getParity();
}

void QUaModbusClientQmlContext::setParity(const int& parity)
{
	Q_ASSERT(m_client);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead || !m_canWrite)
	{
		return;
	}
#endif // QUA_ACCESS_CONTROL
	auto serial = qobject_cast<QUaModbusRtuSerialClient*>(m_client);
	if (!serial)
	{
		return;
	}
	return serial->setParity(static_cast<QParity>(parity));
}

QBaudRate QUaModbusClientQmlContext::baudRate() const
{
	Q_ASSERT(m_client);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead)
	{
		return QBaudRate::UnknownBaud;
	}
#endif // QUA_ACCESS_CONTROL
	auto serial = qobject_cast<QUaModbusRtuSerialClient*>(m_client);
	if (!serial)
	{
		return QBaudRate::UnknownBaud;
	}
	return serial->getBaudRate();
}

void QUaModbusClientQmlContext::setBaudRate(const int& baudRate)
{
	Q_ASSERT(m_client);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead || !m_canWrite)
	{
		return;
	}
#endif // QUA_ACCESS_CONTROL
	auto serial = qobject_cast<QUaModbusRtuSerialClient*>(m_client);
	if (!serial)
	{
		return;
	}
	return serial->setBaudRate(static_cast<QBaudRate>(baudRate));
}

QDataBits QUaModbusClientQmlContext::dataBits() const
{
	Q_ASSERT(m_client);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead)
	{
		return QDataBits::UnknownDataBits;
	}
#endif // QUA_ACCESS_CONTROL
	auto serial = qobject_cast<QUaModbusRtuSerialClient*>(m_client);
	if (!serial)
	{
		return QDataBits::UnknownDataBits;
	}
	return serial->getDataBits();
}

void QUaModbusClientQmlContext::setDataBits(const int& dataBits)
{
	Q_ASSERT(m_client);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead || !m_canWrite)
	{
		return;
	}
#endif // QUA_ACCESS_CONTROL
	auto serial = qobject_cast<QUaModbusRtuSerialClient*>(m_client);
	if (!serial)
	{
		return;
	}
	return serial->setDataBits(static_cast<QDataBits>(dataBits));
}

QStopBits QUaModbusClientQmlContext::stopBits() const
{
	Q_ASSERT(m_client);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead)
	{
		return QStopBits::UnknownStopBits;
	}
#endif // QUA_ACCESS_CONTROL
	auto serial = qobject_cast<QUaModbusRtuSerialClient*>(m_client);
	if (!serial)
	{
		return QStopBits::UnknownStopBits;
	}
	return serial->getStopBits();
}

void QUaModbusClientQmlContext::setStopBits(const int& stopBits)
{
	Q_ASSERT(m_client);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead || !m_canWrite)
	{
		return;
	}
#endif // QUA_ACCESS_CONTROL
	auto serial = qobject_cast<QUaModbusRtuSerialClient*>(m_client);
	if (!serial)
	{
		return;
	}
	return serial->setStopBits(static_cast<QStopBits>(stopBits));
}

QVariantMap QUaModbusClientQmlContext::blocks()
{
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead)
	{
		return QVariantMap();
	}
#endif // QUA_ACCESS_CONTROL
	return m_blocks;
}

QVariant QUaModbusClientQmlContext::blocksModel()
{
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead)
	{
		return QVariant();
	}
#endif // QUA_ACCESS_CONTROL
	QList<QObject*> retList;
	for (auto &blockVariant : m_blocks)
	{
		auto context = blockVariant.value<QUaModbusDataBlockQmlContext*>();
#ifdef QUA_ACCESS_CONTROL
		if (!context->canRead())
		{
			continue;
		}
#endif // QUA_ACCESS_CONTROL
		retList << context;
	}
	return QVariant::fromValue(retList);
}

void QUaModbusClientQmlContext::bindClient(QUaModbusClient* client)
{
	// check valid arg
	Q_ASSERT(client);
	if (!client) { return; }
	// copy reference
	m_client = client;
#ifdef QUA_ACCESS_CONTROL
	QObject::connect(m_client, &QUaModbusClient::permissionsObjectChanged, this,
		[this]() {
			this->updatePermissions();
		});
#endif // QUA_ACCESS_CONTROL
	// susbcribe to changes
	// QUaModbusClient
	QObject::connect(m_client, &QUaModbusClient::serverAddressChanged , this, &QUaModbusClientQmlContext::serverAddressChanged );
	QObject::connect(m_client, &QUaModbusClient::keepConnectingChanged, this, &QUaModbusClientQmlContext::keepConnectingChanged);
	QObject::connect(m_client, &QUaModbusClient::stateChanged         , this, &QUaModbusClientQmlContext::stateChanged         );
	QObject::connect(m_client, &QUaModbusClient::lastErrorChanged     , this, &QUaModbusClientQmlContext::lastErrorChanged     );
	// QUaModbusDataBlockList
	this->bindBlocks(m_client->dataBlocks());
	// QUaModbusTcpClient
	auto tcp = qobject_cast<QUaModbusTcpClient*>(m_client);
	if (tcp)
	{
		QObject::connect(tcp, &QUaModbusTcpClient::networkAddressChanged, this, &QUaModbusClientQmlContext::networkAddressChanged);
		QObject::connect(tcp, &QUaModbusTcpClient::networkPortChanged   , this, &QUaModbusClientQmlContext::networkPortChanged);
		return;
	}
	// QUaModbusRtuSerialClient
	auto serial = qobject_cast<QUaModbusRtuSerialClient*>(m_client);
	if (serial)
	{
		QObject::connect(serial, &QUaModbusRtuSerialClient::comPortChanged , this, &QUaModbusClientQmlContext::comPortChanged );
		QObject::connect(serial, &QUaModbusRtuSerialClient::parityChanged  , this, &QUaModbusClientQmlContext::parityChanged  );
		QObject::connect(serial, &QUaModbusRtuSerialClient::baudRateChanged, this, &QUaModbusClientQmlContext::baudRateChanged);
		QObject::connect(serial, &QUaModbusRtuSerialClient::dataBitsChanged, this, &QUaModbusClientQmlContext::dataBitsChanged);
		QObject::connect(serial, &QUaModbusRtuSerialClient::stopBitsChanged, this, &QUaModbusClientQmlContext::stopBitsChanged);
		return;
	}
}

void QUaModbusClientQmlContext::clear()
{
	// delete children contexts
	while (!m_blocks.isEmpty())
	{
		auto context = m_blocks.take(m_blocks.firstKey()).value<QUaModbusDataBlockQmlContext*>();
		context->clear();
		delete context;
	}
	// clear block qml refs
	m_blocks.clear();
	emit this->blocksChanged();
}

#ifdef QUA_ACCESS_CONTROL
QUaUser* QUaModbusClientQmlContext::loggedUser() const
{
	return m_loggedUser;
}
#endif

void QUaModbusClientQmlContext::connect()
{
	Q_ASSERT(m_client);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead || !m_canWrite)
	{
		return;
	}
#endif // QUA_ACCESS_CONTROL
	m_client->connectDevice();
}

void QUaModbusClientQmlContext::disconnect()
{
	Q_ASSERT(m_client);
#ifdef QUA_ACCESS_CONTROL
	if (!m_canRead || !m_canWrite)
	{
		return;
	}
#endif // QUA_ACCESS_CONTROL
	m_client->disconnectDevice();
}

#ifdef QUA_ACCESS_CONTROL
void QUaModbusClientQmlContext::updateLoggedUser(QUaUser* loggedUser)
{
	// notify downstream
	m_loggedUser = loggedUser;
	emit this->userChanged();
	// update members
	this->updatePermissions();
}

void QUaModbusClientQmlContext::updatePermissions()
{
	auto perms = m_client->permissionsObject();
	m_canRead = !perms ? true : perms->canUserRead(m_loggedUser);
	emit this->canReadChanged();
	m_canWrite = !perms ? true : perms->canUserWrite(m_loggedUser);
	emit this->canWriteChanged();
	// update models
	// NOTE : also triggers blocksModelChanged
	emit this->blocksChanged();
}
#endif // QUA_ACCESS_CONTROL

void QUaModbusClientQmlContext::bindBlocks(QUaModbusDataBlockList* blocks)
{
	// check valid arg
	Q_ASSERT(blocks);
	if (!blocks) { return; }
	// bind about to clear
	QObject::connect(blocks, &QUaModbusDataBlockList::aboutToClear, this,
	[this]() {
		this->clear();
	});
	// bind existing
	for (auto block : blocks->blocks())
	{
		// bind existing block
		this->bindBlock(block);
	}
	// bind block added
	QObject::connect(blocks, &QUaNode::childAdded, this,
	[this](QUaNode* node) {
		// bind new block
		auto block = qobject_cast<QUaModbusDataBlock*>(node);
		Q_ASSERT(block);
		this->bindBlock(block);
	}/*, Qt::QueuedConnection // NOTE : do not queue or blocks will not be available on view load */);
}

void QUaModbusClientQmlContext::bindBlock(QUaModbusDataBlock* block)
{
	// get id
	QString strId = block->browseName().name();
	Q_ASSERT(!strId.isEmpty() && !strId.isNull());
	// add context to map
	auto context = new QUaModbusDataBlockQmlContext(this);
	context->bindBlock(block);
	Q_ASSERT(!m_blocks.contains(strId));
	m_blocks[strId] = QVariant::fromValue(context);
#ifdef QUA_ACCESS_CONTROL
	QObject::connect(this, &QUaModbusClientQmlContext::userChanged, context, [this, context]() {
		context->updateLoggedUser(m_loggedUser);
	});
#endif // QUA_ACCESS_CONTROL
	// subscribe to destroyed
	QObject::connect(block, &QObject::destroyed, context,
	[this, block]() {
		this->removeBlock(block);
	});
	// notify changes
	emit this->blocksChanged();
}

void QUaModbusClientQmlContext::removeBlock(QUaModbusDataBlock* block)
{
	QString strId = block->browseName().name();
	Q_ASSERT(m_blocks.contains(strId));
	delete m_blocks.take(strId).value<QUaModbusDataBlockQmlContext*>();
	// notify changes
	emit this->blocksChanged();
}

/******************************************************************************************************
*/

QUaModbusQmlContext::QUaModbusQmlContext(QObject *parent) : QObject(parent)
{
#ifdef QUA_ACCESS_CONTROL
	m_loggedUser = nullptr;
#endif // QUA_ACCESS_CONTROL
	// forward signal
	QObject::connect(this, &QUaModbusQmlContext::clientsChanged, this, &QUaModbusQmlContext::clientsModelChanged);
	if (QMetaType::type("QModbusClientType") == QMetaType::UnknownType)
	{
		qRegisterMetaType<QModbusClientType>("QModbusClientType");
	}
	if (QMetaType::type("QModbusState") == QMetaType::UnknownType)
	{
		qRegisterMetaType<QModbusState>("QModbusState");
	}
	if (QMetaType::type("QModbusError") == QMetaType::UnknownType)
	{
		qRegisterMetaType<QModbusError>("QModbusError");
	}
	if (QMetaType::type("QParity") == QMetaType::UnknownType)
	{
		qRegisterMetaType<QParity>("QParity");
	}
	if (QMetaType::type("QBaudRate") == QMetaType::UnknownType)
	{
		qRegisterMetaType<QBaudRate>("QBaudRate");
	}
	if (QMetaType::type("QDataBits") == QMetaType::UnknownType)
	{
		qRegisterMetaType<QDataBits>("QDataBits");
	}
	if (QMetaType::type("QStopBits") == QMetaType::UnknownType)
	{
		qRegisterMetaType<QStopBits>("QStopBits");
	}
	if (QMetaType::type("QModbusDataBlockType") == QMetaType::UnknownType)
	{
		qRegisterMetaType<QModbusDataBlockType>("QModbusDataBlockType");
	}
	if (QMetaType::type("QModbusValueType") == QMetaType::UnknownType)
	{
		qRegisterMetaType<QModbusValueType>("QModbusValueType");
	}
}


QUaModbusQmlContext::~QUaModbusQmlContext()
{

}

QVariantMap QUaModbusQmlContext::clients()
{
	return m_clients;
}

QVariant QUaModbusQmlContext::clientsModel()
{
	QList<QObject*> retList;
	for (auto &clientVariant : m_clients)
	{
		retList << clientVariant.value<QUaModbusClientQmlContext*>();
	}
	return QVariant::fromValue(retList);
}

void QUaModbusQmlContext::bindClients(QUaModbusClientList* clients)
{
	// check valid arg
	Q_ASSERT(clients);
	if (!clients) { return; }
	// clear previous if any
	this->clear();
	// un-bind previous if any
	while (!m_connections.isEmpty())
	{
		QObject::disconnect(m_connections.takeFirst());
	}
	// bind about to clear
	m_connections <<
	QObject::connect(clients, &QUaModbusClientList::aboutToClear, this,
	[this]() {
		this->clear();
	});
	// bind existing
	for (auto client : clients->clients())
	{
		// bind existing client
		this->bindClient(client);
	}
	// bind client added
	m_connections << QObject::connect(clients, &QUaNode::childAdded, this,
		[this](QUaNode* node) {
			// bind new client
			QUaModbusClient* client = qobject_cast<QUaModbusClient*>(node);
			Q_ASSERT(client);
			this->bindClient(client);
		}/*, Qt::QueuedConnection // NOTE : do not queue or clients will not be available on view load */);
}

void QUaModbusQmlContext::clear()
{
	// delete client contexts
	while (!m_clients.isEmpty())
	{
		auto context = m_clients.take(m_clients.firstKey()).value<QUaModbusClientQmlContext*>();
		context->clear();
		delete context;
	}
	// clear client qml refs
	m_clients.clear();
	emit this->clientsChanged();
}

#ifdef QUA_ACCESS_CONTROL
QUaUser* QUaModbusQmlContext::loggedUser() const
{
	return m_loggedUser;
}

void QUaModbusQmlContext::on_loggedUserChanged(QUaUser* user)
{
	// notify downstream
	m_loggedUser = user;
	emit this->userChanged();
}
#endif

void QUaModbusQmlContext::bindClient(QUaModbusClient* client)
{
	// get client id
	QString strId = client->browseName().name();
	Q_ASSERT(!strId.isEmpty() && !strId.isNull());
	// add client context to map
	auto context = new QUaModbusClientQmlContext(this);
	context->bindClient(client);
	Q_ASSERT(!m_clients.contains(strId));
	m_clients[strId] = QVariant::fromValue(context);
#ifdef QUA_ACCESS_CONTROL
	QObject::connect(this, &QUaModbusQmlContext::userChanged, context, [this, context]() {
		context->updateLoggedUser(m_loggedUser);
	});
#endif // QUA_ACCESS_CONTROL
	// subscribe to destroyed
	QObject::connect(client, &QObject::destroyed, context,
	[this, client]() {
		this->removeClient(client);
	});
	// notify changes
	emit this->clientsChanged();
}

void QUaModbusQmlContext::removeClient(QUaModbusClient* client)
{
	QString strId = client->browseName().name();
	Q_ASSERT(m_clients.contains(strId));
	auto context = m_clients.take(strId).value<QUaModbusClientQmlContext*>();
	delete context;
	// notify changes
	emit this->clientsChanged();
}

