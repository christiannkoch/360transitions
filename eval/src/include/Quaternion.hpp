/*
	Author: Xavier Corbillon
	IMT Atlantique
*/
#pragma once
#include <iostream>
#include <map>
#include <vector>
#include <cmath>

#include "Vector.hpp"

SCALAR PI_L = 3.141592653589793238462643383279502884L;

namespace IMT {

	struct not_unit_quaternion_exception : std::exception
	{
		char const* what() const throw() { return "Rotation require unit quaternion"; }
	};

	class Quaternion
	{
	public:
		Quaternion(void) : m_w(0), m_v(), m_isNormalized(false) {}
		Quaternion(const SCALAR& w, const VectorCartesian& v) : m_w(w), m_v(v), m_isNormalized(false) {}
		Quaternion(const SCALAR& w, const SCALAR& x, const SCALAR& y, const SCALAR& z) : m_w(w), m_v(x, y, z), m_isNormalized(false) {}
		explicit Quaternion(const SCALAR& w) : m_w(w), m_v(), m_isNormalized(false) {}
		explicit Quaternion(const VectorCartesian& v) : m_w(0), m_v(v), m_isNormalized(false) {}
		static Quaternion FromEuler(const SCALAR& yaw, const SCALAR& pitch, const SCALAR& roll)
		{
			double t0 = std::cos(yaw * 0.5);
			double t1 = std::sin(yaw * 0.5);
			double t2 = std::cos(roll * 0.5);
			double t3 = std::sin(roll * 0.5);
			// double t4 = std::cos( (pitch - PI_L/2.0) * 0.5);
			// double t5 = std::sin( (pitch - PI_L/2.0) * 0.5);
			double t4 = std::cos(pitch * 0.5);
			double t5 = std::sin(pitch * 0.5);

			auto q = Quaternion(t0 * t2 * t4 + t1 * t3 * t5, VectorCartesian(t0 * t3 * t4 - t1 * t2 * t5,
				t0 * t2 * t5 + t1 * t3 * t4,
				t1 * t2 * t4 - t0 * t3 * t5));
			q.Normalize();
			return q;
		}
		VectorCartesian ToEuler() const
		{
			double sinr = 2.0 * (m_w * m_v.GetX() + m_v.GetY() * m_v.GetZ());
			double cosr = 1.0 - 2.0 * (m_v.GetX() * m_v.GetX() + m_v.GetY() * m_v.GetY());
			auto roll = atan2(sinr, cosr);

			// pitch (y-axis rotation)
			double sinp = 2.0 * (m_w * m_v.GetY() - m_v.GetZ() * m_v.GetX());
			double pitch;
			if (fabs(sinp) >= 1)
				pitch = copysign(PI_L / 2, sinp); // use 90 degrees if out of range
			else
				pitch = asin(sinp);

			// yaw (z-axis rotation)
			double siny = 2.0 * (m_w * m_v.GetZ() + m_v.GetX() * m_v.GetY());
			double cosy = 1.0 - 2.0 * (m_v.GetY() * m_v.GetY() + m_v.GetZ() * m_v.GetZ());
			auto yaw = atan2(siny, cosy);

			return { roll, pitch, yaw };
		}

		SCALAR DotProduct(const Quaternion& q) const { return m_w * q.m_w + m_v.DotProduct(q.m_v); }
		const SCALAR Norm(void) const { return std::sqrt(DotProduct(*this)); }

		Quaternion operator+(const Quaternion& q) const
		{
			return Quaternion(m_w + q.m_w, m_v + q.m_v);
		}
		Quaternion operator+(const VectorCartesian& v) const
		{
			return this->operator+(Quaternion(v));
		}
		Quaternion operator-(const Quaternion& q) const
		{
			return Quaternion(m_w - q.m_w, m_v - q.m_v);
		}
		Quaternion operator-(void) const
		{
			return Quaternion(-m_w, -m_v);
		}
		Quaternion operator-(const VectorCartesian& v) const
		{
			return this->operator-(Quaternion(v));
		}
		Quaternion operator*(const Quaternion& q) const
		{
			return Quaternion((m_w*q.m_w) - (m_v*q.m_v),
				(m_w*q.m_v) + (q.m_w*m_v) + (m_v ^ q.m_v));
		}
		Quaternion operator*(const VectorCartesian& v) const
		{
			return this->operator*(Quaternion(v));
		}
		Quaternion operator*(SCALAR s) const
		{
			return Quaternion(m_w*s, m_v*s);
		}
		Quaternion operator/(const SCALAR& s) const
		{
			return Quaternion(m_w / s, m_v / s);
		}

		std::ostream& operator<<(std::ostream& o) const
		{
			o << m_w << " + " << m_v.GetX() << " i + " << m_v.GetY() << " j + " << m_v.GetZ() << " k ";
			return o;
		}

		// Quaternion& operator=(const Quaternion& q)
		// {
		//   m_w = q.m_w;
		//   m_v = q.m_v;
		//   return *this;
		// }

		bool operator==(const Quaternion& q) const
		{
			return m_w == q.m_w && m_v == q.m_v;
		}
		bool operator!=(const Quaternion& q) const
		{
			return !(*this == q);
		}

		void Normalize(void)
		{
			if (!m_isNormalized)
			{
				*this = *this / Norm();
				m_isNormalized = true;
			}
		}

		Quaternion Normalized(void)
		{
			return !m_isNormalized ? (*this / Norm()) : *this;
		}

		SCALAR GetW(void) const { return m_w; }
		VectorCartesian GetV(void) const { return m_v; }

		bool IsPur(void) const { return m_w == 0; }
		Quaternion Conj(void) const { return Quaternion(m_w, -m_v); }
		Quaternion Inv(void) const { return m_isNormalized ? Conj() : Conj() / std::pow(Norm(), 2); }
		VectorCartesian Rotation(const VectorCartesian& v) const
		{
			return m_isNormalized ?
				((*this)*v*(this->Conj())).GetV() :
				((*this)*v*(this->Conj()) / std::pow(this->Norm(), 2)).GetV();
		}

		static  Quaternion Exp(const Quaternion& q)
		{
			return Quaternion(std::cos(q.m_v.Norm())*std::exp(q.m_w),
				q.m_v.Norm() != 0 ? std::sin(q.m_v.Norm()) * (q.m_v / q.m_v.Norm()) : q.m_v
			);
		}
		static  Quaternion Log(const Quaternion& q)
		{
			return Quaternion(std::log(q.Norm()),
				q.m_v.Norm() != 0 && q.Norm() != 0 ? std::acos(q.m_w / q.Norm())*(q.m_v / q.m_v.Norm()) : q.m_v
			);
		}
		static const SCALAR Distance(const Quaternion& q1, const Quaternion& q2)
		{
			return (q2 - q1).Norm();
		}

		static SCALAR OrthodromicDistance(const Quaternion& q1, const Quaternion& q2)
		{
			auto origine = VectorCartesian(1, 0, 0);
			Quaternion p1 = Quaternion(q1.Rotation(origine));
			Quaternion p2 = Quaternion(q2.Rotation(origine));
			auto p = p1 * p2;
			// p1 and p2 are pur so -p.m_w is the dot product and p.m_v is the vector product of p1 and p2
			return std::atan2(p.m_v.Norm(), -p.m_w);
		}

		static  Quaternion pow(const Quaternion& q, const SCALAR& k)
		{
			return Quaternion::Exp(Quaternion::Log(q) * k);
		}

		static Quaternion SLERP(const Quaternion& q1, const Quaternion& q2, const SCALAR& k)
		{
			if (q1.DotProduct(q2) < 0)
			{
				return q1 * Quaternion::pow(q1.Inv() * (-q2), k);
			}
			else
			{
				return q1 * Quaternion::pow(q1.Inv() * q2, k);
			}
		}

		static  Quaternion QuaternionFromAngleAxis(const SCALAR& theta, const VectorCartesian& u)
		{
			return Quaternion(std::cos(theta / 2), std::sin(theta / 2)*(u / u.Norm()));
		}

		static VectorCartesian AverageAngularVelocity(Quaternion q1, Quaternion q2, const SCALAR& deltaT)
		{
			if (q1.DotProduct(q2) < 0)
			{
				q2 = -q2;
			}
			if (!q1.IsPur())
			{
				if (!q1.m_isNormalized)
				{
					q1.Normalize();
				}
				q1 = Quaternion(q1.Rotation(VectorCartesian(1, 0, 0)));
			}
			if (!q2.IsPur())
			{
				if (!q2.m_isNormalized)
				{
					q2.Normalize();
				}
				q2 = Quaternion(q2.Rotation(VectorCartesian(1, 0, 0)));
			}
			auto deltaQ = q2 - q1;
			auto W = (deltaQ * (2.0 / deltaT))*q1.Inv();
			return W.m_v;
		}

	private:
		SCALAR m_w;
		VectorCartesian m_v;
		bool m_isNormalized;
	};

	inline std::ostream& operator<<(std::ostream& o, const Quaternion& q) { return q.operator<<(o); }
	inline  Quaternion operator+(const SCALAR& s, const Quaternion& q) { return Quaternion(s) + q; }
	inline  Quaternion operator-(const SCALAR& s, const Quaternion& q) { return Quaternion(s) - q; }
	inline  Quaternion operator+(const VectorCartesian& v, const Quaternion& q) { return Quaternion(v) + q; }
	inline  Quaternion operator-(const VectorCartesian& v, const Quaternion& q) { return Quaternion(v) - q; }
	inline  Quaternion operator+(const VectorCartesian& v, const SCALAR& s) { return Quaternion(v) + Quaternion(s); }
	inline  Quaternion operator-(const VectorCartesian& v, const SCALAR& s) { return Quaternion(v) - Quaternion(s); }
	inline  Quaternion operator*(const SCALAR& s, const Quaternion& q) { return q * s; }
	inline  Quaternion operator*(const VectorCartesian& v, const Quaternion& q) { return Quaternion(v) * q; }
	inline  Quaternion operator*(const Quaternion& q, const VectorCartesian& v) { return q * Quaternion(v); }

	// inline  Quaternion pow(const Quaternion& q, const SCALAR& k)
	// {
	//   return Quaternion::pow(q, k);
	// }
}
