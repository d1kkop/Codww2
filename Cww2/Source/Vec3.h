#pragma once

struct Vec4;


struct Vec3
{
	union
	{
		struct { float x, y, z; };
		struct { float m[3]; };
	};
	Vec3():x(0),y(0),z(0) { }
	Vec3(float _x, float _y, float _z): 
		x(_x), y(_y), z(_z) { }
	Vec3(const Vec4& v);
	bool operator == (const Vec3& o) const
	{
		return x==o.x && y==o.y && z==o.z;
	}
	float dist(const Vec3& o) const
	{
		float dx = x-o.x;
		float dy = y-o.y;
		float dz = z-o.z;
		return sqrt(dx*dx + dy*dy + dz*dz);
		//return sqrtf(dot(dv));
	}
	float length() const { return sqrtf(x*x+y*y+z*z); }
	Vec3& normalize() { float k = 1.f/length(); return (*this *= k); }
	Vec3& operator*= (float f) 
	{
		x *= f;
		y *= f;
		z *= f;
		return *this;
	}
	Vec3& operator+= (const Vec3& o)
	{
		x += o.x;
		y += o.y;
		z += o.z;
		return *this;
	}
	Vec3& operator-= (const Vec3& o)
	{
		x -= o.x;
		y -= o.y;
		z -= o.z;
		return *this;
	}
	Vec3 operator+ (const Vec3& o) const
	{
		Vec3 r = *this;
		r += o;
		return r;
	}
	Vec3 operator- (const Vec3& o) const
	{
		Vec3 r = *this;
		r -= o;
		return r;
	}
	Vec3 operator* (float f) const
	{
		Vec3 r = *this;
		r.x *= f;
		r.y *= f;
		r.z *= f;
		return r;
	}
	float dot(const Vec3& o) const
	{
		return x*o.x + y*o.y + z*o.z;
	}
	Vec3 cross(const Vec3& o) const
	{
		Vec3 r;
		r.x = y*o.z - z*o.y;
		r.y = z*o.x - x*o.z;
		r.z = x*o.y - y*o.x;
		return r;
	}
};


struct Vec4
{
	union
	{
		struct { float x, y, z, w; };
		struct { float m[4]; };
	};
	Vec4():x(0),y(0),z(0),w(0) { }
	Vec4(float _x, float _y, float _z, float _w): 
		x(_x), y(_y), z(_z), w(_w) { }
	bool operator == (const Vec4& o) const
	{
		return x==o.x && y==o.y && z==o.z && w==o.w;
	}
	float length() const { return sqrtf(x*x+y*y+z*z+w*w); }
	Vec3 operator* (const Vec3& v) const
	{
		Vec3 q(x,y,z);
		Vec3 t =  q.cross(v) * 2.f;
		return v + t * w + q.cross(t);
	}
};



inline Vec3::Vec3(const Vec4& v):Vec3(v.x,v.y,v.z) { }