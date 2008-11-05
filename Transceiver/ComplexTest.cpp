/*
* Copyright (c) 2008, Kestrel Signal Processing, Inc.
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

/*
Contributors:
Harvind S. Samra, hssamra@kestrelsp.com
*/


#include "Complex.h"

#include <iostream>
using namespace std;

int main(int argc, char*argv[])
{

	Complex<float> za(1,1);
	Complex<float> zb(1,0);
	Complex<float> zc(0,1);


	cout << "za=" << za << endl;
	cout << "za*za=" << za*za << endl;
	cout << "za*zb=" << za*zb << endl;
	cout << "za*zc=" << za*zc << endl;
	cout << "za^5=" << za.pow(5) << endl;

}
