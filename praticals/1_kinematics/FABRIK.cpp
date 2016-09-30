#include "ik.h"
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <iostream>
#include <phys_utils.h>
#include <vector>
using namespace std;
using namespace glm;
static unsigned int numLinks = 0;

void GetRotation(int i, std::vector<Link> &const links, glm::vec3 newEndPos, glm::vec3 prePos)
{
	dquat qCur = angleAxis(links[i].m_angle, links[i].m_axis);
	vec3 vlinkBasePos = (links[i].m_base)[3];

	vec3 oldVec = normalize(prePos - vlinkBasePos);
	vec3 newVec = normalize(newEndPos - vlinkBasePos);

	vec3 axisVec = cross(newVec, oldVec);
	float angleVec = angle(newVec, oldVec);

	dquat qNew = normalize(angleAxis(angleVec, axisVec));

	qCur *= qNew;
	qCur = normalize(qCur);//should be able to collapse this somehow

	links[i].m_angle = angle(qCur);
	links[i].m_axis = axis(qCur);
	UpdateHierarchy();
}


void FABRIK_Update(const vec3 &const target, std::vector<Link> &const links, const float linkLength)
{
	numLinks = links.size();
	const float distance = length(vec3(links[0].m_end[3]) - target);

	if (distance > (linkLength * links.size()))
	{
		//target is unreachable
		for (int i = 0; i < (numLinks - 1); i++)
		{
			vec3 prePos = vec3(links[i + 1].m_end[3]);
			float r = length(target - vec3(links[i].m_end[3]));
			float lambda = linkLength / r;

			vec3 newEndPos = (1 - lambda) * (vec3(links[i].m_end[3]) + (lambda * target));
			GetRotation(i + 1, links, newEndPos, prePos);
		}
	}
	else
	{
		vec3 rootInitial = vec3(links[0].m_end[3]);

		float targetDelta = length(vec3(links[numLinks - 1].m_end[3]) - target);

		while (targetDelta > 0.5f)
		{
			vec3(links[numLinks - 1].m_end[3]) = target;

			for (int i = (numLinks - 2); i > 0; i--)
			{

				vec3 prePos = links[i].m_end[3];
				float r = length(vec3(links[i + 1].m_end[3]) - vec3(links[i].m_end[3]));
				float lambda = linkLength / r;
				vec3 newEndPos = (1 - lambda) * (vec3(links[i + 1].m_end[3]) + (lambda * vec3(links[i].m_end[3])));
				GetRotation(i, links, newEndPos, prePos);
			}

			vec3(links[0].m_end[3]) = rootInitial;

			for (int i = 0; i < (numLinks - 1); i++)
			{
				vec3 prePos = vec3(links[i].m_end[3]);
				float r = length(vec3(links[i + 1].m_end[3]) - vec3(links[i].m_end[3]));
				float lambda = linkLength / r;
				vec3 newEndPos = (1 - lambda) * (vec3(links[i].m_end[3])) + (lambda * vec3(links[i + 1].m_end[3]));
				GetRotation(i, links, newEndPos, prePos);
			}
			targetDelta = length(vec3(links[numLinks - 1].m_end[3]) - target);
		}
	}
	return;
}