#include "collision.h"
#include "cPhysicsComponents.h"
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
using namespace std;
using namespace glm;
namespace collision
{
	bool PointInRange(double point, double rp1, double rp2)
	{
		bool inRange = false;
		double min = 0.0;
		double max = 0.0;
		if (rp1 <= rp2)
		{
			min = rp1;
			max = rp2;
		}
		else
		{
			min = rp2;
			max = rp1;
		}

		if (min <= point)
			if (point <= max)
				inRange = true;

		return inRange;
	}


	bool IsCollidingCheck(std::vector<collisionInfo> &civ, const cSphereCollider &c1, const cSphereCollider &c2)
	{
		
		const dvec3 p1 = c1.GetParent()->GetPosition();
		const dvec3 p2 = c2.GetParent()->GetPosition();

		if (length(p1 - p2) <= c1.radius + c2.radius)//could reduce, should reduce, but shits broke anyway...
		{
			const dvec3 d = p2 - p1;
			const double distance = glm::length(d);
			const double sumRadius = c1.radius + c2.radius;
			if (distance < sumRadius) {
				auto depth = sumRadius - distance;
				auto norm = -glm::normalize(d);
				auto pos = p1 - norm * (c1.radius - depth * 0.5f);
				civ.push_back({ &c1, &c2, pos, norm, depth });
				return true;
			}
		}
		return false;
	}

	bool IsCollidingCheck(std::vector<collisionInfo> &civ, const cSphereCollider &s, const cPlaneCollider &p) {
		const dvec3 sp = s.GetParent()->GetPosition();
		const dvec3 pp = p.GetParent()->GetPosition();

		// Calculate a vector from a point on the plane to the center of the sphere
		const dvec3 vecTemp(sp - pp);

		// Calculate the distance: dot product of the new vector with the plane's normal
		double distance = dot(vecTemp, p.normal);
		dvec3 collPos = sp - p.normal * distance;//gives pos directly below...

		dvec3 d = (sp - s.GetParent()->GetPrevPosition());
		dvec3 reflect = d - (2.0 * dot(d, normalize(p.normal))*normalize(p.normal));
		reflect += (normalize(p.normal));
		if (distance <= s.radius) {
			civ.push_back({ &s, &p, collPos, normalize(reflect), (s.radius - distance)*50.0});//scaled because otherwise this is reduced too much to bounce, will likely want to create bounciness variable for rigidbodies, this however means that this scales with velocity so objects will increase in speed the more they bounce, also as objects at steep angle are going "faster" they will go faster than shallow angled ones
			return true;
		}

		return false;
	}

	bool IsCollidingCheck(std::vector<collisionInfo> &civ, const cSphereCollider &c1, const cBoxCollider &c2) {
		const dvec3 sp = c1.GetParent()->GetPosition();
		const dvec3 bp = c2.GetParent()->GetPosition();
		bool isCollided = false;

		if (length(sp - bp) <= c1.radius + c2.radius)
		{
			double c2rad = c2.radius / 2.0;

			dvec3 c2Points[8] = { dvec3(c2rad, c2rad, c2rad),   dvec3(-c2rad, c2rad, c2rad),
				dvec3(c2rad, -c2rad, c2rad),  dvec3(-c2rad, -c2rad, c2rad),
				dvec3(c2rad, c2rad, -c2rad),  dvec3(-c2rad, c2rad, -c2rad),
				dvec3(c2rad, -c2rad, -c2rad), dvec3(-c2rad, -c2rad, -c2rad) };

			const mat4 c2m = glm::translate(bp) * mat4_cast(c2.GetParent()->GetRotation());
			for (int i = 0; i < 8; i++)
			{
				c2Points[i] = dvec3(c2m * dvec4(c2Points[i], 1.0));
			}

			for (int i = 0; i < 8; i++)//Check if box 1 point i is in box 2
			{
				if (length(c2Points[i] - sp) < c1.radius)
				{
					cout << length(c2Points[i] - sp) << endl;
					double depth = c1.radius - length(c2Points[i] - sp);
					civ.push_back({ &c1, &c2, c2Points[i], normalize(sp - c2Points[i]) , depth });
					isCollided = true;
				}
			}
		}
		return isCollided;
	}

	bool IsCollidingCheck(std::vector<collisionInfo> &civ, const cPlaneCollider &c1, const cPlaneCollider &c2) {
		//I won're remove this stub, but when will this ever be useful?
		cout << "PLANE PLANE" << endl;
		return false;
	}

	bool IsCollidingCheck(std::vector<collisionInfo> &civ, const cPlaneCollider &p, const cBoxCollider &b) {
		const dvec3 pp = p.GetParent()->GetPosition();
		const dvec3 bp = b.GetParent()->GetPosition();

		double brad = b.radius / 2.0;
		// local coords on cube
		dvec3 points[8] = { dvec3(brad, brad, brad),   dvec3(-brad, brad, brad),
			dvec3(brad, -brad, brad),  dvec3(-brad, -brad, brad),
			dvec3(brad, brad, -brad),  dvec3(-brad, brad, -brad),
			dvec3(brad, -brad, -brad), dvec3(-brad, -brad, -brad) };

		// transfrom to global
		const mat4 m = glm::translate(bp) * mat4_cast(b.GetParent()->GetRotation());
		for (int i = 0; i < 8; i++)
		{
			points[i] = dvec3(m * dvec4(points[i], 1.0));
		}

		// For each point on the cube, which side of cube are they on?
		double distances[8];
		bool isCollided = false;
		for (int i = 0; i < 8; i++) {
			dvec3 planeNormal = p.normal;

			distances[i] = dot(pp, planeNormal) - dot(points[i], planeNormal);

			if (distances[i] > 0)
			{
				distances[i] * 1.3;
				civ.push_back({ &p, &b, points[i] + planeNormal * distances[i], planeNormal, distances[i]});
				isCollided = true;
			}
		}
		return isCollided;
	}

	bool IsCollidingCheck(std::vector<collisionInfo> &civ, const cBoxCollider &c1, const cBoxCollider &c2)
	{
		const dvec3 c1p = c1.GetParent()->GetPosition();
		const dvec3 c2p = c2.GetParent()->GetPosition();
		bool isCollided = false;

		if (length(c1p - c2p) <= c1.radius + c2.radius)
		{
			double c1rad = c1.radius / 2.0;
			double c2rad = c2.radius / 2.0;

			dvec3 c1Points[8] = { dvec3(c1rad, c1rad, c1rad),   dvec3(-c1rad, c1rad, c1rad),
				dvec3(c1rad, -c1rad, c1rad),  dvec3(-c1rad, -c1rad, c1rad),
				dvec3(c1rad, c1rad, -c1rad),  dvec3(-c1rad, c1rad, -c1rad),
				dvec3(c1rad, -c1rad, -c1rad), dvec3(-c1rad, -c1rad, -c1rad) };


			dvec3 c2Points[8] = { dvec3(c2rad, c2rad, c2rad),   dvec3(-c2rad, c2rad, c2rad),
				dvec3(c2rad, -c2rad, c2rad),  dvec3(-c2rad, -c2rad, c2rad),
				dvec3(c2rad, c2rad, -c2rad),  dvec3(-c2rad, c2rad, -c2rad),
				dvec3(c2rad, -c2rad, -c2rad), dvec3(-c2rad, -c2rad, -c2rad) };

			const mat4 c1m = glm::translate(c1p) * mat4_cast(c1.GetParent()->GetRotation());
			const mat4 c2m = glm::translate(c2p) * mat4_cast(c2.GetParent()->GetRotation());
			for (int i = 0; i < 8; i++)
			{
				c1Points[i] = dvec3(c1m * dvec4(c1Points[i], 1.0));
				c2Points[i] = dvec3(c2m * dvec4(c2Points[i], 1.0));
			}



			for (int i = 0; i < 8; i++)//Check if box 1 point i is in box 2
			{
				if (PointInRange(c1Points[i].x, c2Points[0].x, c2Points[7].x))
					if (PointInRange(c1Points[i].y, c2Points[0].y, c2Points[7].y))
						if (PointInRange(c1Points[i].z, c2Points[0].z, c2Points[7].z))
						{
							dvec3 d = c2p - c1p;
							double distance = length(d);
							double depth = (length(c1p - c1Points[i]) + length(c2p - c2Points[i]) - distance)*0.1;
							vec3 pos = c2p - normalize(d) * ((length(c1Points[i] - c1p)) - depth);
							civ.push_back({ &c1, &c2, pos, normalize(d) , depth });
							//cout << normalize(dif) << ", " << depth << endl;
							isCollided = true;
						}
			}

			for (int i = 0; i < 8; i++)//Check if box 2 point i is in box 1
			{
				if (PointInRange(c2Points[i].x, c1Points[0].x, c1Points[7].x))
					if (PointInRange(c2Points[i].y, c1Points[0].y, c1Points[7].y))
						if (PointInRange(c2Points[i].z, c1Points[0].z, c1Points[7].z))
						{
							dvec3 d = c1p - c2p;
							double distance = length(d);
							double depth = (length(c1p - c1Points[i]) + length(c2p - c2Points[i]) - distance)*0.1;
							vec3 pos = c1p - normalize(d) * ((length(c2p - c2Points[i])) - depth);
							civ.push_back({ &c1, &c2, pos, normalize(d) , depth });
							//cout << normalize(dif) << ", " << depth << endl;
							isCollided = true;
						}
			}
		}
		return isCollided;
	}

	bool IsColliding(std::vector<collisionInfo> &civ, const cCollider &c1, const cCollider &c2) {
		enum shape { UNKOWN = 0, PLANE, SPHERE, BOX };
		shape s1 = UNKOWN;
		shape s2 = UNKOWN;
		if (dynamic_cast<const cSphereCollider *>(&c1)) {
			s1 = SPHERE;
		}
		else if (dynamic_cast<const cPlaneCollider *>(&c1)) {
			s1 = PLANE;
		}
		else if (dynamic_cast<const cBoxCollider *>(&c1)) {
			s1 = BOX;
		}

		if (dynamic_cast<const cSphereCollider *>(&c2)) {
			s2 = SPHERE;
		}
		else if (dynamic_cast<const cPlaneCollider *>(&c2)) {
			s2 = PLANE;
		}
		else if (dynamic_cast<const cBoxCollider *>(&c2)) {
			s2 = BOX;
		}

		if (!s1 || !s2) {
			cout << "Routing Error" << endl;
			return false;
		}
		if (s1 == PLANE) {
			if (s2 == PLANE) {
				return IsCollidingCheck(civ, dynamic_cast<const cPlaneCollider &>(c1), dynamic_cast<const cPlaneCollider &>(c2));
			}
			else if (s2 == SPHERE) {
				return IsCollidingCheck(civ, dynamic_cast<const cSphereCollider &>(c2), dynamic_cast<const cPlaneCollider &>(c1));
			}
			else if (s2 == BOX) {
				return IsCollidingCheck(civ, dynamic_cast<const cPlaneCollider &>(c1), dynamic_cast<const cBoxCollider &>(c2));
			}
			else {
				cout << "Routing Error" << endl;
				return false;
			}
		}
		else if (s1 == SPHERE) {
			if (s2 == PLANE) {
				return IsCollidingCheck(civ, dynamic_cast<const cSphereCollider &>(c1), dynamic_cast<const cPlaneCollider &>(c2));
			}
			else if (s2 == SPHERE) {
				return IsCollidingCheck(civ, dynamic_cast<const cSphereCollider &>(c1),
					dynamic_cast<const cSphereCollider &>(c2));
			}
			else if (s2 == BOX) {
				return IsCollidingCheck(civ, dynamic_cast<const cSphereCollider &>(c1), dynamic_cast<const cBoxCollider &>(c2));
			}
			else {
				cout << "Routing Error" << endl;
				return false;
			}
		}
		else if (s1 == BOX) {
			if (s2 == PLANE) {
				return IsCollidingCheck(civ, dynamic_cast<const cPlaneCollider &>(c2), dynamic_cast<const cBoxCollider &>(c1));
			}
			else if (s2 == SPHERE) {
				return IsCollidingCheck(civ, dynamic_cast<const cSphereCollider &>(c2), dynamic_cast<const cBoxCollider &>(c1));
			}
			else if (s2 == BOX) {
				return IsCollidingCheck(civ, dynamic_cast<const cBoxCollider &>(c2), dynamic_cast<const cBoxCollider &>(c1));
			}
			else {
				cout << "Routing Error" << endl;
				return false;
			}
		}
		return false;
	}
}