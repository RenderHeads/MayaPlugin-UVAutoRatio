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

#ifndef UVSPRINGLAYOUT_H
#define UVSPRINGLAYOUT_H

#include <iostream>
#include <vector>

class Box;
class Spring;

class Box
{
public:
	Box();
	~Box();

	double width, height;

	double2 position;
	double2 velocity;
	double2 force;
	double2 dp, dv;

	std::vector<Spring*> springs;

	bool IsConnected(const Box& box) const;
};

class Spring
{
public:
	Box* from;
	Box* to;
	double constant;
	double damping;
	double restLength;
};

class UVSpringLayout
{
public:
	UVSpringLayout();
	~UVSpringLayout();

	void	Clear();
	void	AddBox(double width, double height, const double2& center);
	bool	Step(double stepSize);
	void	GetPosition(uint index, double2& position);

private:
	void	ConnectOverlapping();
	void	RemoveSprings();
	void	UpdateParticles(double time);
	void	CalculateForces();
	void	CalculateDerivatives();

	bool	BoxIntersect(const Box& a, const Box& b) const;

	std::vector<Box*> m_boxes;
	std::vector<Spring*> m_springs;
};


#endif

