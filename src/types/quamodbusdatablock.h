#ifndef QUAMODBUSDATABLOCK_H
#define QUAMODBUSDATABLOCK_H

#include <QModbusDataUnit>
#include <QModbusReply>

#ifndef QUA_ACCESS_CONTROL
#include <QUaBaseObject>
#else
#include <QUaBaseObjectProtected>
#endif // !QUA_ACCESS_CONTROL

#include <QDomDocument>
#include <QDomElement>

class QUaModbusClient;
class QUaModbusDataBlockList;
class QUaModbusValue;

#include "quamodbusvaluelist.h"

typedef QModbusDevice::State QModbusState;
typedef QModbusDevice::Error QModbusError;

#ifndef QUA_ACCESS_CONTROL
class QUaModbusDataBlock : public QUaBaseObject
#else
class QUaModbusDataBlock : public QUaBaseObjectProtected
#endif // !QUA_ACCESS_CONTROL
{
	friend class QUaModbusDataBlockList;
	friend class QUaModbusValue;

    Q_OBJECT

	// UA properties
	Q_PROPERTY(QUaProperty * Type         READ type        )
	Q_PROPERTY(QUaProperty * Address      READ address     )
	Q_PROPERTY(QUaProperty * Size         READ size        )
	Q_PROPERTY(QUaProperty * SamplingTime READ samplingTime)

	// UA variables
	Q_PROPERTY(QUaBaseDataVariable * Data      READ data     )
	Q_PROPERTY(QUaBaseDataVariable * LastError READ lastError)

	// UA objects
	Q_PROPERTY(QUaModbusValueList * Values READ values)

public:
	Q_INVOKABLE explicit QUaModbusDataBlock(QUaServer *server);
	~QUaModbusDataBlock();

	// register as Q_ENUM
	enum RegisterType 
	{
		Invalid          = QModbusDataUnit::RegisterType::Invalid         ,
		DiscreteInputs   = QModbusDataUnit::RegisterType::DiscreteInputs  ,
		Coils            = QModbusDataUnit::RegisterType::Coils           ,
		InputRegisters   = QModbusDataUnit::RegisterType::InputRegisters  ,
		HoldingRegisters = QModbusDataUnit::RegisterType::HoldingRegisters
	};
	Q_ENUM(RegisterType)
	typedef QUaModbusDataBlock::RegisterType QModbusDataBlockType;

	// UA properties

	QUaProperty * type        ();
	QUaProperty * address     ();
	QUaProperty * size        ();
	QUaProperty * samplingTime();

	// UA variables

	QUaBaseDataVariable * data();
	QUaBaseDataVariable * lastError();

	// UA objects

	QUaModbusValueList * values();

	// UA methods

	Q_INVOKABLE void remove();

	// C++ API (all is read/write)

	QModbusDataBlockType getType() const;
	void                 setType(const QModbusDataBlockType &type);

	int  getAddress() const;
	void setAddress(const int &address);

	quint32 getSize() const;
	void    setSize(const quint32 &size);

	quint32 getSamplingTime() const;
	void    setSamplingTime(const quint32 &samplingTime);

	QVector<quint16> getData() const;
	void             setData(const QVector<quint16> &data, const bool &writeModbus = true);

	QModbusError getLastError() const;
	void         setLastError(const QModbusError &error);

	bool isWellConfigured() const;

	QUaModbusDataBlockList * list() const;

	QUaModbusClient * client() const;

signals:
	// C++ API
	void typeChanged        (const QModbusDataBlockType &type        );
	void addressChanged     (const int                  &address     );
	void sizeChanged        (const quint32              &size        );
	void samplingTimeChanged(const quint32              &samplingTime);
	void dataChanged        (const QVector<quint16>     &data        );
	void lastErrorChanged   (const QModbusError         &error       );

	// (internal) to safely update error in ua server thread
	void updateLastError(const QModbusError &error);
	void aboutToDestroy();

private slots:
	// handle UA change events (also reused in C++ API and triggers C++ API events)
	void on_typeChanged        (const QVariant     &value, const bool &networkChange);
	void on_addressChanged     (const QVariant     &value, const bool &networkChange);
	void on_sizeChanged        (const QVariant     &value, const bool &networkChange);
	void on_samplingTimeChanged(const QVariant     &value, const bool &networkChange);
	void on_dataChanged        (const QVariant     &value, const bool &networkChange);
	void on_updateLastError    (const QModbusError &error);

private:
	int  m_loopHandle;
	bool m_firstSample;
	QModbusReply  * m_replyRead;
	// NOTE : only modify and access in thread
	QModbusDataBlockType m_registerType;
	int                  m_startAddress;
	quint32              m_valueCount;

	void startLoop();
	bool loopRunning();
	void setModbusData(const QVector<quint16>& data);

	// XML import / export
	QDomElement toDomElement  (QDomDocument & domDoc) const;
	void        fromDomElement(QDomElement  & domElem, QQueue<QUaLog>& errorLogs);

	static quint32 m_minSamplingTime;
	static QVector<quint16> variantToInt16Vect(const QVariant &value);

	QUaProperty* m_type;
	QUaProperty* m_address;
	QUaProperty* m_size;
	QUaProperty* m_samplingTime;
	QUaBaseDataVariable* m_data;
	QUaBaseDataVariable* m_lastError;
	QUaModbusValueList* m_values;
};

typedef QUaModbusDataBlock::RegisterType QModbusDataBlockType;

#endif // QUAMODBUSDATABLOCK_H
