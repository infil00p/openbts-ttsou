/*
* Copyright 2008 Free Software Foundation, Inc.
*
* This software is distributed under the terms of the GNU Public License.
* See the COPYING file in the main directory for details.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/




#include "Timeval.h"
#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{

	Timeval then(10000);
	cout << then.elapsed() << endl;

	while (!then.passed()) {
		cout << "now: " << Timeval() << " then: " << then << " remaining: " << then.remaining() << endl;
		usleep(500000);
	}
	cout << "now: " << Timeval() << " then: " << then << " remaining: " << then.remaining() << endl;
}