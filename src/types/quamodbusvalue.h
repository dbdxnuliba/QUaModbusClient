#ifndef QUAMODBUSVALUE_H
#define QUAMODBUSVALUE_H

#include <QModbusDataUnit>
#include <QModbusReply>

#ifndef QUA_ACCESS_CONTROL
#include <QUaBaseObject>
#else
#include <QUaBaseObjectProtected>
#endif // !QUA_ACCESS_CONTROL

#include <QDomDocument>
#include <QDomElement>

class QUaModbusDataBlock;
class QUaModbusValueList;
class QUaModbusClient;

typedef QModbusDevice::Error QModbusError;

#ifndef QUA_ACCESS_CONTROL
class QUaModbusValue : public QUaBaseObject
#else
class QUaModbusValue : public QUaBaseObjectProtected
#endif // !QUA_ACCESS_CONTROL
{
	friend class QUaModbusValueList;
	friend class QUaModbusDataBlock;

    Q_OBJECT

	// UA properties
	Q_PROPERTY(QUaProperty * Type              READ type             )
	Q_PROPERTY(QUaProperty * RegistersUsed     READ registersUsed    )
	Q_PROPERTY(QUaProperty * AddressOffset     READ addressOffset    )
#ifndef QUAMODBUS_NOCYCLIC_WRITE
	Q_PROPERTY(QUaProperty * CyclicWritePeriod READ cyclicWritePeriod)
	Q_PROPERTY(QUaProperty * CyclicWriteMode   READ cyclicWriteMode  )
#endif // !QUAMODBUS_NOCYCLIC_WRITE
	// UA variables
	Q_PROPERTY(QUaBaseDataVariable * Value     READ value    )
	Q_PROPERTY(QUaBaseDataVariable * LastError READ lastError)

public:
	Q_INVOKABLE explicit QUaModbusValue(QUaServer *server);
	~QUaModbusValue();

	enum ValueType
	{
		Invalid        = -1,
		Binary0        = 0,
		Binary1        = 1,
		Binary2        = 2,
		Binary3        = 3,
		Binary4        = 4,
		Binary5        = 5,
		Binary6        = 6,
		Binary7        = 7,
		Binary8        = 8,
		Binary9        = 9,
		Binary10       = 10,
		Binary11       = 11,
		Binary12       = 12,
		Binary13       = 13,
		Binary14       = 14,
		Binary15       = 15,
		Decimal        = 16,
		Int            = 17, // i32 Least Significant Register First
		IntSwapped     = 18, // i32 Most Significant Register First
		Float          = 19, // f32 Least Significant Register First
		FloatSwapped   = 20, // f32 Most Significant Register First
		Int64          = 21, // i64 Least Significant Register First
		Int64Swapped   = 22, // i64 Most Significant Register First
		Float64        = 23, // f64 Least Significant Register First
		Float64Swapped = 24, // f64 Most Significant Register First
	};
	Q_ENUM(ValueType)
	typedef QUaModbusValue::ValueType QModbusValueType;

	// UA properties

	QUaProperty * type();
	QUaProperty * registersUsed();
	QUaProperty * addressOffset();

	// UA variables

	QUaBaseDataVariable * value();
	QUaBaseDataVariable * lastError();

	// UA methods

	Q_INVOKABLE void remove();

	// C++ API
	QModbusValueType getType() const;
	void             setType(const QModbusValueType &type);

	quint16 getRegistersUsed() const;

	int  getAddressOffset() const;
	void setAddressOffset(const int &addressOffset);

	QVariant getValue() const;
	void     setValue(const QVariant &value);

#ifndef QUAMODBUS_NOCYCLIC_WRITE
	enum CyclicWriteMode
	{
		Current   = 0, // most recent value
		Toggle    = 1, // negate current value
		Increase  = 2, // add one to current value
		Decrease  = 3  // substract one from current value
	};
	Q_ENUM(CyclicWriteMode)
	typedef QUaModbusValue::CyclicWriteMode QModbusCyclicWriteMode;

	QUaProperty* cyclicWritePeriod();
	QUaProperty* cyclicWriteMode();

	quint32 getCyclicWritePeriod() const;
	void setCyclicWritePeriod(const quint32& cyclicWritePeriod);

	QModbusCyclicWriteMode getCyclicWriteMode() const;
	void setCyclicWriteMode(const QModbusCyclicWriteMode& cyclicWriteMode);
#endif // !QUAMODBUS_NOCYCLIC_WRITE

	QModbusError getLastError() const;
	void         setLastError(const QModbusError &error);

	bool isWellConfigured() const;

	bool isWritable() const;

	QUaModbusValueList * list() const;

	QUaModbusDataBlock * block() const;

	QUaModbusClient    * client() const;

	static int              typeBlockSize(const QModbusValueType &type);
	static QMetaType::Type  typeToMeta   (const QModbusValueType &type);
	static QVariant         blockToValue (const QVector<quint16> &block, const QModbusValueType &type);
	static QVector<quint16> valueToBlock (const QVariant         &value, const QModbusValueType &type);

signals:
	// C++ API
	void typeChanged         (const QModbusValueType &type         );
	void registersUsedChanged(const quint16          &registersUsed);
	void addressOffsetChanged(const int              &addressOffset);
	void valueChanged        (const QVariant         &value        );
	void lastErrorChanged    (const QModbusError     &error        );
	// (internal) to safely update error in ua server thread
	void updateLastError(const QModbusError &error);
	void aboutToDestroy();

#ifndef QUAMODBUS_NOCYCLIC_WRITE
	void cyclicWritePeriodChanged(const quint32& cyclicWritePeriod);
	void cyclicWriteModeChanged  (const QModbusCyclicWriteMode& cyclicWriteMode);
	// (internal) used by cycle in thread
	void cyclicWrite();
#endif // !QUAMODBUS_NOCYCLIC_WRITE

private slots:
	void on_typeChanged             (const QVariant     &value, const bool& networkChange);
	void on_addressOffsetChanged    (const QVariant     &value, const bool& networkChange);
	void on_valueChanged            (const QVariant     &value, const bool& networkChange);
	void on_updateLastError         (const QModbusError &error);
#ifndef QUAMODBUS_NOCYCLIC_WRITE
	void on_cyclicWritePeriodChanged(const QVariant     &value, const bool& networkChange);
	void on_cyclicWriteModeChanged  (const QVariant     &value, const bool& networkChange);
	// cyclic write with last value
	void on_cyclicWrite();
#endif // !QUAMODBUS_NOCYCLIC_WRITE

private:
	int m_loopId;
	bool m_wellConfigured;
	QModbusValueType m_typeCache;
	int m_addressOffsetCache;
	QModbusError m_lastErrorCache;
	QUaProperty* m_type;
	QUaProperty* m_registersUsed;
	QUaProperty* m_addressOffset;
#ifndef QUAMODBUS_NOCYCLIC_WRITE
	QUaProperty* m_cyclicWritePeriod;
	QUaProperty* m_cyclicWriteMode;
#endif // !QUAMODBUS_NOCYCLIC_WRITE
	QUaBaseDataVariable* m_value;
	QUaBaseDataVariable* m_lastError;

	void setValue(const QVector<quint16> &block, const QModbusError &blockError, const bool forceIfSame = false);

	void updateWellConfigured(const QModbusValueType& type, const int& addressOffset);

	// XML import / export
	QDomElement toDomElement  (QDomDocument & domDoc) const;
	void        fromDomElement(QDomElement  & domElem, QQueue<QUaLog>& errorLogs);
	
};

typedef QUaModbusValue::ValueType QModbusValueType;
#ifndef QUAMODBUS_NOCYCLIC_WRITE
typedef QUaModbusValue::CyclicWriteMode QModbusCyclicWriteMode;
#endif // !QUAMODBUS_NOCYCLIC_WRITE

#endif // QUAMODBUSVALUE_H
