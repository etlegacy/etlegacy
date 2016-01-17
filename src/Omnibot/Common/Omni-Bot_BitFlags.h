////////////////////////////////////////////////////////////////////////////////
//
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#ifndef INCLUDE_USERFLAGS_H
#define INCLUDE_USERFLAGS_H

#include "Omni-Bot_BasicTypes.h"

class BitFlag32
{
public:
	bool AnyFlagSet() const
	{
		return m_Flags != 0;
	}
	bool CheckFlag(obint32 _flag) const
	{
		return (m_Flags & ((obint32)1 << _flag)) != 0;
	}
	void SetFlag(obint32 _flag)
	{
		m_Flags |= ((obint32)1 << _flag);
	}
	void SetFlag(obint32 _flag, bool _set)
	{
		if (_set)
		{
			SetFlag(_flag);
		}
		else
		{
			ClearFlag(_flag);
		}
	}
	void ClearFlag(obint32 _flag)
	{
		m_Flags &= ~((obint32)1 << _flag);
	}
	void ClearAll()
	{
		m_Flags = 0;
	}
	BitFlag32& operator&=(const BitFlag32& rhs)
	{
		m_Flags &= rhs.m_Flags;
		return *this;
	}
	BitFlag32 operator&(const BitFlag32& rhs) const
	{
		BitFlag32 bf(*this);
		bf &= rhs;
		return bf;
	}
	BitFlag32& operator|=(const BitFlag32& rhs)
	{
		m_Flags |= rhs.m_Flags;
		return *this;
	}
	BitFlag32 operator|(const BitFlag32& rhs) const
	{
		BitFlag32 bf(*this);
		bf |= rhs;
		return bf;
	}
	BitFlag32 operator~() const
	{
		BitFlag32 bf(*this);
		bf.m_Flags = ~bf.m_Flags;
		return bf;
	}
	bool operator<(obint32 _rhs) const
	{
		return m_Flags < _rhs;
	}
	bool operator==(const BitFlag32& r) const
	{
		return m_Flags == r.m_Flags;
	}
	bool operator!=(const BitFlag32& r) const
	{
		return m_Flags != r.m_Flags;
	}
	obint32 GetRawFlags() const
	{
		return m_Flags;
	}
	explicit BitFlag32(obint32 flgs = 0) : m_Flags(flgs)
	{
	}
private:
	obint32 m_Flags;
};

class BitFlag64
{
public:
	bool AnyFlagSet() const
	{
		return m_Flags != 0;
	}
	bool CheckFlag(obint32 _flag) const
	{
		obint64 flg = (obint64)1 << _flag;
		return (m_Flags & flg) != 0;
	}
	void SetFlag(obint32 _flag)
	{
		m_Flags |= ((obint64)1 << _flag);
	}
	void SetFlag(obint32 _flag, bool _set)
	{
		if (_set)
		{
			SetFlag(_flag);
		}
		else
		{
			ClearFlag(_flag);
		}
	}
	void ClearFlag(obint32 _flag)
	{
		m_Flags &= ~((obint64)1 << _flag);
	}
	void ClearAll()
	{
		m_Flags = 0;
	}
	BitFlag64& operator&=(const BitFlag64& rhs)
	{
		m_Flags &= rhs.m_Flags;
		return *this;
	}
	BitFlag64 operator&(const BitFlag64& rhs) const
	{
		BitFlag64 bf(*this);
		bf &= rhs;
		return bf;
	}
	BitFlag64& operator|=(const BitFlag64& rhs)
	{
		m_Flags |= rhs.m_Flags;
		return *this;
	}
	BitFlag64 operator|(const BitFlag64& rhs) const
	{
		BitFlag64 bf(*this);
		bf |= rhs;
		return bf;
	}
	BitFlag64 operator~() const
	{
		BitFlag64 bf(*this);
		bf.m_Flags = ~bf.m_Flags;
		return bf;
	}
	bool operator<(obint64 _rhs) const
	{
		return m_Flags < _rhs;
	}
	bool operator==(const BitFlag64& r) const
	{
		return m_Flags == r.m_Flags;
	}
	bool operator!=(const BitFlag64& r) const
	{
		return m_Flags != r.m_Flags;
	}
	obint64 GetRawFlags() const
	{
		return m_Flags;
	}
	explicit BitFlag64(obint64 flgs = 0) : m_Flags(flgs)
	{
	}
private:
	obint64 m_Flags;
};

class BitFlag128
{
public:
	bool AnyFlagSet() const
	{
		return (m_Lo | m_Hi) != 0;
	}
	bool CheckFlag(obint32 _flag) const
	{
		if (_flag < 64)
		{
			return (m_Lo & ((obint64)1 << _flag)) != 0;
		}
		else
		{
			return (m_Hi & ((obint64)1 << (_flag - 64))) != 0;
		}
	}
	void SetFlag(obint32 _flag)
	{
		if (_flag < 64)
		{
			m_Lo |= ((obint64)1 << _flag);
		}
		else
		{
			m_Hi |= ((obint64)1 << (_flag - 64));
		}
	}
	void SetFlag(obint32 _flag, bool _set)
	{
		if (_set)
		{
			SetFlag(_flag);
		}
		else
		{
			ClearFlag(_flag);
		}
	}
	void ClearFlag(obint32 _flag)
	{
		if (_flag < 64)
		{
			m_Lo &= ~((obint64)1 << _flag);
		}
		else
		{
			m_Hi &= ~((obint64)1 << (_flag - 64));
		}
	}
	void ClearAll()
	{
		m_Lo = m_Hi = 0;
	}
	BitFlag128 operator&(const BitFlag128& rhs) const
	{
		return BitFlag128(m_Lo & rhs.m_Lo, m_Hi & rhs.m_Hi);
	}
	explicit BitFlag128(obint64 lo = 0, obint64 hi = 0) : m_Lo(lo), m_Hi(hi)
	{
	}
private:
	obint64 m_Lo, m_Hi;
};


#endif
