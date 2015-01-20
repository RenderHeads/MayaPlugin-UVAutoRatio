//
// UVAutoRatio Maya Plugin Source Code
// Copyright (C) 2007-2014 RenderHeads Ltd.
//
// This source is available for distribution and/or modification
// only under the terms of the MIT license.  All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the MIT license
// for more details.
//

#include "MayaPCH.h"
#include <algorithm>
#include "MayaUtility.h"
#include "Utility.h"
#include "UVSpringLayout.h"

using namespace std;

bool
Box::IsConnected(const Box& box) const
{
	for (uint i = 0; i < springs.size(); i++)
	{
		const Spring& s = *springs[i];
		if (s.to == &box || s.from == &box)
		{
			return true;
		}
	}
	return false;
}

Box::Box()
{
	width = height = 0.0;
	position[0] = position[1] = 0.0;
	velocity[0] = velocity[1] = 0.0;
	force[0] = force[1] = 0.0;
	dp[0] = dp[1] = 0.0;
	dv[0] = dv[1] = 0.0;
	springs.reserve(256);
}

Box::~Box()
{
	springs.clear();
}

UVSpringLayout::UVSpringLayout()
{
	m_boxes.reserve(256);
	m_springs.reserve(2048);
}

UVSpringLayout::~UVSpringLayout()
{
	Clear();
}

void
UVSpringLayout::Clear()
{
	// Delete all boxes
	{
		std::vector<Box*>::reverse_iterator riter;
		for ( riter = m_boxes.rbegin(); riter != m_boxes.rend(); ++riter )
		{
			delete (*riter);
		}
		m_boxes.clear();
	}

	// Delete all springs
	{
		std::vector<Spring*>::reverse_iterator riter;
		for ( riter = m_springs.rbegin(); riter != m_springs.rend(); ++riter )
		{
			delete (*riter);
		}
		m_springs.clear();
	}
}

void
UVSpringLayout::AddBox(double width, double height, const double2& center)
{
	Box* b = new Box();
	b->width = width;
	b->height = height;
	b->position[0] = center[0];
	b->position[1] = center[1];
	m_boxes.push_back(b);
}

void
UVSpringLayout::GetPosition(uint index, double2& position)
{
	position[0] = m_boxes[index]->position[0];
	position[1] = m_boxes[index]->position[1];
}

bool
UVSpringLayout::Step(double stepSize)
{
	ConnectOverlapping();
	RemoveSprings();
	UpdateParticles(stepSize);

	size_t num = m_boxes.size();
	size_t i = 0;
	for (i = 0; i < num; i++)
	{
		Box& box = *m_boxes[i];
		if (box.springs.size() == 0)
		{
			box.velocity[0] = box.velocity[1] = 0.0;
			box.force[0] = box.force[1] = 0.0;
		}
	}

	// TODO: we could just check whether there are any springs instead of this check
	bool intersections = (m_springs.size() != 0);
	/*bool intersections = false;
	for (i = 0; i < num; i++)
	{
		Box& boxA = *m_boxes[i];

		for (size_t j = i + 1; j < num; j++)
		{
			Box& boxB = *m_boxes[j];
			if (BoxIntersect(boxA, boxB))
			{
				intersections = true;
				break;
			}
		}
	}*/

	return intersections;
}

bool
UVSpringLayout::BoxIntersect(const Box& a, const Box& b) const
{
	double border = 1.0 / 256.0;

	double aLeft = a.position[0] - (a.width * 0.5) - border;
	double bLeft = b.position[0] - (b.width * 0.5) - border;
	double aRight = a.position[0] + (a.width * 0.5) + border;
	double bRight = b.position[0] + (b.width * 0.5) + border;

	double aTop = a.position[1] - (a.height * 0.5) - border;
	double bTop = b.position[1] - (b.height * 0.5) - border;
	double aBottom = a.position[1] + (a.height * 0.5) + border;
	double bBottom = b.position[1] + (b.height * 0.5) + border;

	if (aRight <= bLeft)
		return false;

	if (aLeft >= bRight)
		return false;

	if (aBottom <= bTop)
		return false;

	if (aTop >= bBottom)
		return false;

	return true;
}

void
UVSpringLayout::ConnectOverlapping()
{
	size_t num = m_boxes.size();
	for (size_t i = 0; i < num; i++)
	{
		Box& boxA = *m_boxes[i];
		for (size_t j = i + 1; j < num; j++)
		{
			Box& boxB = *m_boxes[j];
			if (BoxIntersect(boxA, boxB))
			{
				// check if already connected
				if (!boxA.IsConnected(boxB))
				{
					Spring* ss = new Spring();
					ss->from = &boxA;
					ss->to = &boxB;
					ss->constant = 0.125;
					ss->damping = 0.01;
					double h1 = 0.75 * sqrt((boxA.width * boxA.width) + (boxA.height * boxA.height));
					double h2 = 0.75 * sqrt((boxB.width *  boxB.width) + ( boxB.height * boxB.height));
					ss->restLength = h1 + h2;
					boxA.springs.push_back(ss);
					boxB.springs.push_back(ss);

					m_springs.push_back(ss);
				}
			}
		}
	}
}

void
UVSpringLayout::RemoveSprings()
{
	std::vector<Spring*> springstoRemove;
	uint i = 0;
	for (i = 0; i < m_springs.size(); i++)
	{
		Spring& s = *m_springs[i];
		Box& boxA = *s.from;
		Box& boxB = *s.to;
		if (!BoxIntersect(boxA, boxB))
		{
			// Try find the spring
			vector<Spring*>::iterator itA = std::find(boxA.springs.begin(), boxA.springs.end(), &s);
			vector<Spring*>::iterator itB = std::find(boxB.springs.begin(), boxB.springs.end(), &s);

			assert(itA != boxA.springs.end());
			assert(itB != boxB.springs.end());

			boxA.springs.erase(itA);
			boxB.springs.erase(itB);

			springstoRemove.push_back(&s);
		}
	}

	for (i = 0; i < springstoRemove.size(); i++)
	{
		Spring& s = *springstoRemove[i];

		vector<Spring*>::iterator itA = std::find(m_springs.begin(), m_springs.end(), &s);
		assert(itA != m_springs.end());
		m_springs.erase(itA);
	}
}

void
UVSpringLayout::UpdateParticles(double dt)
{
	CalculateForces();
	CalculateDerivatives();

	for (uint i = 0; i < m_boxes.size(); i++)
	{
		Box& box = *m_boxes[i];
		box.position[0] += box.dp[0] * dt;
		box.position[1] += box.dp[1] * dt;
		box.velocity[0] += box.dv[0] * dt;
		box.velocity[1] += box.dv[1] * dt;
	}
}

void
UVSpringLayout::CalculateForces()
{
	double drag = 0.1;
	uint i = 0;
	for (i = 0; i < m_boxes.size(); i++)
	{
		Box& box = *m_boxes[i];

		box.force[0] = box.force[1] = 0.0;

		box.force[0] -= drag * box.velocity[0];
		box.force[1] -= drag * box.velocity[1];
	}

	for (i = 0; i < m_springs.size(); i++)
	{
		Spring& s = *m_springs[i];

		Box& p1 = *s.from;
		Box& p2 = *s.to;

		double len = 0.0;
		double2 d;
		d[0] = p1.position[0] - p2.position[0];
		d[1] = p1.position[1] - p2.position[1];
		double lensqr = (d[0] * d[0]) + (d[1] * d[1]);
		if (lensqr != 0.0)
		{
			len = sqrt(lensqr);
			d[0] /= len;
			d[1] /= len;
		}
		else
		{
			p1.position[0] += RandomSignedUnit() * 0.001;
			p1.position[1] += RandomSignedUnit() * 0.001;
			p2.position[0] += RandomSignedUnit() * 0.001;
			p2.position[1] += RandomSignedUnit() * 0.001;
		}

		double2 f;
		f[0] = s.constant * (len - s.restLength);
		f[0] += s.damping * (p1.velocity[0] - p2.velocity[0]) * d[0];
		f[0] *= -d[0];
		f[1] = s.constant * (len - s.restLength);
		f[1] += s.damping * (p1.velocity[1] - p2.velocity[1]) * d[1];
		f[1] *= -d[1];

		p1.force[0] += f[0];
		p1.force[1] += f[1];
		p2.force[0] -= f[0];
		p2.force[1] -= f[1];
	}
}

void
UVSpringLayout::CalculateDerivatives()
{
	for (uint i = 0; i < m_boxes.size(); i++)
	{
		Box& box = *m_boxes[i];
		box.dp[0] = box.velocity[0];
		box.dp[1] = box.velocity[1];
		box.dv[0] = box.force[0];     //  / box.mass;
		box.dv[1] = box.force[1];     //  / box.mass;
	}
}

