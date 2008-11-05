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

/*
Contributors:
Harvind S. Samra, hssamra@kestrelsp.com
*/


float sendLPF_961[] = { -0.000422,-0.000408,-0.000394,-0.000379,-0.000364,-0.000348,-0.000332,-0.000315,-0.000298,-0.000280,-0.000262,-0.000243,-0.000224,-0.000205,-0.000185,-0.000165,-0.000145,-0.000125,-0.000104,-0.000083,-0.000062,-0.000040,-0.000019,0.000003,0.000025,0.000047,0.000069,0.000091,0.000113,0.000135,0.000157,0.000179,0.000200,0.000222,0.000244,0.000265,0.000286,0.000307,0.000328,0.000348,0.000368,0.000388,0.000407,0.000426,0.000445,0.000463,0.000481,0.000498,0.000515,0.000531,0.000547,0.000562,0.000576,0.000590,0.000604,0.000616,0.000628,0.000640,0.000650,0.000660,0.000669,0.000678,0.000686,0.000693,0.000699,0.000704,0.000709,0.000712,0.000715,0.000717,0.000719,0.000719,0.000718,0.000717,0.000715,0.000712,0.000708,0.000703,0.000698,0.000691,0.000684,0.000676,0.000667,0.000657,0.000646,0.000634,0.000622,0.000609,0.000595,0.000580,0.000565,0.000548,0.000531,0.000513,0.000495,0.000476,0.000456,0.000435,0.000414,0.000392,0.000370,0.000347,0.000323,0.000299,0.000275,0.000250,0.000224,0.000199,0.000172,0.000146,0.000119,0.000091,0.000064,0.000036,0.000008,-0.000020,-0.000048,-0.000077,-0.000105,-0.000134,-0.000163,-0.000191,-0.000220,-0.000248,-0.000277,-0.000305,-0.000333,-0.000361,-0.000388,-0.000415,-0.000442,-0.000469,-0.000495,-0.000521,-0.000546,-0.000571,-0.000595,-0.000619,-0.000642,-0.000665,-0.000687,-0.000708,-0.000729,-0.000749,-0.000768,-0.000786,-0.000803,-0.000820,-0.000836,-0.000851,-0.000865,-0.000878,-0.000890,-0.000901,-0.000911,-0.000920,-0.000928,-0.000935,-0.000941,-0.000946,-0.000950,-0.000953,-0.000954,-0.000955,-0.000954,-0.000952,-0.000949,-0.000945,-0.000940,-0.000933,-0.000926,-0.000917,-0.000907,-0.000896,-0.000884,-0.000871,-0.000856,-0.000841,-0.000824,-0.000806,-0.000788,-0.000768,-0.000747,-0.000725,-0.000702,-0.000678,-0.000653,-0.000627,-0.000600,-0.000572,-0.000543,-0.000514,-0.000483,-0.000452,-0.000420,-0.000388,-0.000354,-0.000320,-0.000285,-0.000250,-0.000214,-0.000178,-0.000141,-0.000103,-0.000066,-0.000027,0.000011,0.000050,0.000089,0.000128,0.000167,0.000207,0.000246,0.000286,0.000326,0.000365,0.000404,0.000444,0.000483,0.000521,0.000560,0.000598,0.000636,0.000673,0.000710,0.000746,0.000782,0.000817,0.000851,0.000884,0.000917,0.000949,0.000981,0.001011,0.001040,0.001068,0.001096,0.001122,0.001147,0.001171,0.001194,0.001216,0.001236,0.001255,0.001273,0.001289,0.001304,0.001318,0.001330,0.001341,0.001350,0.001358,0.001364,0.001368,0.001371,0.001373,0.001372,0.001370,0.001367,0.001362,0.001355,0.001346,0.001336,0.001324,0.001311,0.001295,0.001278,0.001260,0.001239,0.001217,0.001194,0.001168,0.001141,0.001113,0.001083,0.001051,0.001017,0.000982,0.000946,0.000908,0.000869,0.000828,0.000785,0.000742,0.000697,0.000650,0.000603,0.000554,0.000504,0.000453,0.000401,0.000347,0.000293,0.000238,0.000182,0.000125,0.000067,0.000008,-0.000051,-0.000111,-0.000171,-0.000232,-0.000293,-0.000354,-0.000416,-0.000479,-0.000541,-0.000603,-0.000666,-0.000728,-0.000790,-0.000852,-0.000914,-0.000976,-0.001037,-0.001097,-0.001157,-0.001217,-0.001276,-0.001334,-0.001391,-0.001447,-0.001502,-0.001556,-0.001609,-0.001661,-0.001712,-0.001761,-0.001808,-0.001855,-0.001899,-0.001942,-0.001983,-0.002023,-0.002060,-0.002096,-0.002130,-0.002161,-0.002191,-0.002218,-0.002243,-0.002266,-0.002286,-0.002304,-0.002319,-0.002332,-0.002343,-0.002350,-0.002355,-0.002358,-0.002357,-0.002354,-0.002348,-0.002339,-0.002327,-0.002312,-0.002294,-0.002273,-0.002249,-0.002222,-0.002191,-0.002158,-0.002121,-0.002082,-0.002039,-0.001993,-0.001944,-0.001891,-0.001835,-0.001777,-0.001714,-0.001649,-0.001581,-0.001509,-0.001434,-0.001356,-0.001275,-0.001191,-0.001104,-0.001013,-0.000920,-0.000823,-0.000724,-0.000622,-0.000517,-0.000409,-0.000298,-0.000184,-0.000068,0.000051,0.000173,0.000297,0.000424,0.000553,0.000684,0.000818,0.000954,0.001092,0.001232,0.001374,0.001518,0.001664,0.001812,0.001961,0.002112,0.002265,0.002419,0.002574,0.002731,0.002888,0.003047,0.003207,0.003367,0.003529,0.003691,0.003853,0.004016,0.004180,0.004343,0.004507,0.004671,0.004835,0.004999,0.005162,0.005325,0.005488,0.005650,0.005811,0.005972,0.006132,0.006290,0.006448,0.006604,0.006759,0.006913,0.007065,0.007216,0.007364,0.007511,0.007656,0.007799,0.007940,0.008079,0.008215,0.008349,0.008481,0.008609,0.008736,0.008859,0.008980,0.009097,0.009212,0.009323,0.009432,0.009537,0.009638,0.009737,0.009832,0.009923,0.010011,0.010095,0.010175,0.010252,0.010325,0.010394,0.010459,0.010520,0.010577,0.010630,0.010678,0.010723,0.010764,0.010800,0.010832,0.010860,0.010884,0.010903,0.010918,0.010929,0.010935,0.010937,0.010935,0.010929,0.010918,0.010903,0.010884,0.010860,0.010832,0.010800,0.010764,0.010723,0.010678,0.010630,0.010577,0.010520,0.010459,0.010394,0.010325,0.010252,0.010175,0.010095,0.010011,0.009923,0.009832,0.009737,0.009638,0.009537,0.009432,0.009323,0.009212,0.009097,0.008980,0.008859,0.008736,0.008609,0.008481,0.008349,0.008215,0.008079,0.007940,0.007799,0.007656,0.007511,0.007364,0.007216,0.007065,0.006913,0.006759,0.006604,0.006448,0.006290,0.006132,0.005972,0.005811,0.005650,0.005488,0.005325,0.005162,0.004999,0.004835,0.004671,0.004507,0.004343,0.004180,0.004016,0.003853,0.003691,0.003529,0.003367,0.003207,0.003047,0.002888,0.002731,0.002574,0.002419,0.002265,0.002112,0.001961,0.001812,0.001664,0.001518,0.001374,0.001232,0.001092,0.000954,0.000818,0.000684,0.000553,0.000424,0.000297,0.000173,0.000051,-0.000068,-0.000184,-0.000298,-0.000409,-0.000517,-0.000622,-0.000724,-0.000823,-0.000920,-0.001013,-0.001104,-0.001191,-0.001275,-0.001356,-0.001434,-0.001509,-0.001581,-0.001649,-0.001714,-0.001777,-0.001835,-0.001891,-0.001944,-0.001993,-0.002039,-0.002082,-0.002121,-0.002158,-0.002191,-0.002222,-0.002249,-0.002273,-0.002294,-0.002312,-0.002327,-0.002339,-0.002348,-0.002354,-0.002357,-0.002358,-0.002355,-0.002350,-0.002343,-0.002332,-0.002319,-0.002304,-0.002286,-0.002266,-0.002243,-0.002218,-0.002191,-0.002161,-0.002130,-0.002096,-0.002060,-0.002023,-0.001983,-0.001942,-0.001899,-0.001855,-0.001808,-0.001761,-0.001712,-0.001661,-0.001609,-0.001556,-0.001502,-0.001447,-0.001391,-0.001334,-0.001276,-0.001217,-0.001157,-0.001097,-0.001037,-0.000976,-0.000914,-0.000852,-0.000790,-0.000728,-0.000666,-0.000603,-0.000541,-0.000479,-0.000416,-0.000354,-0.000293,-0.000232,-0.000171,-0.000111,-0.000051,0.000008,0.000067,0.000125,0.000182,0.000238,0.000293,0.000347,0.000401,0.000453,0.000504,0.000554,0.000603,0.000650,0.000697,0.000742,0.000785,0.000828,0.000869,0.000908,0.000946,0.000982,0.001017,0.001051,0.001083,0.001113,0.001141,0.001168,0.001194,0.001217,0.001239,0.001260,0.001278,0.001295,0.001311,0.001324,0.001336,0.001346,0.001355,0.001362,0.001367,0.001370,0.001372,0.001373,0.001371,0.001368,0.001364,0.001358,0.001350,0.001341,0.001330,0.001318,0.001304,0.001289,0.001273,0.001255,0.001236,0.001216,0.001194,0.001171,0.001147,0.001122,0.001096,0.001068,0.001040,0.001011,0.000981,0.000949,0.000917,0.000884,0.000851,0.000817,0.000782,0.000746,0.000710,0.000673,0.000636,0.000598,0.000560,0.000521,0.000483,0.000444,0.000404,0.000365,0.000326,0.000286,0.000246,0.000207,0.000167,0.000128,0.000089,0.000050,0.000011,-0.000027,-0.000066,-0.000103,-0.000141,-0.000178,-0.000214,-0.000250,-0.000285,-0.000320,-0.000354,-0.000388,-0.000420,-0.000452,-0.000483,-0.000514,-0.000543,-0.000572,-0.000600,-0.000627,-0.000653,-0.000678,-0.000702,-0.000725,-0.000747,-0.000768,-0.000788,-0.000806,-0.000824,-0.000841,-0.000856,-0.000871,-0.000884,-0.000896,-0.000907,-0.000917,-0.000926,-0.000933,-0.000940,-0.000945,-0.000949,-0.000952,-0.000954,-0.000955,-0.000954,-0.000953,-0.000950,-0.000946,-0.000941,-0.000935,-0.000928,-0.000920,-0.000911,-0.000901,-0.000890,-0.000878,-0.000865,-0.000851,-0.000836,-0.000820,-0.000803,-0.000786,-0.000768,-0.000749,-0.000729,-0.000708,-0.000687,-0.000665,-0.000642,-0.000619,-0.000595,-0.000571,-0.000546,-0.000521,-0.000495,-0.000469,-0.000442,-0.000415,-0.000388,-0.000361,-0.000333,-0.000305,-0.000277,-0.000248,-0.000220,-0.000191,-0.000163,-0.000134,-0.000105,-0.000077,-0.000048,-0.000020,0.000008,0.000036,0.000064,0.000091,0.000119,0.000146,0.000172,0.000199,0.000224,0.000250,0.000275,0.000299,0.000323,0.000347,0.000370,0.000392,0.000414,0.000435,0.000456,0.000476,0.000495,0.000513,0.000531,0.000548,0.000565,0.000580,0.000595,0.000609,0.000622,0.000634,0.000646,0.000657,0.000667,0.000676,0.000684,0.000691,0.000698,0.000703,0.000708,0.000712,0.000715,0.000717,0.000718,0.000719,0.000719,0.000717,0.000715,0.000712,0.000709,0.000704,0.000699,0.000693,0.000686,0.000678,0.000669,0.000660,0.000650,0.000640,0.000628,0.000616,0.000604,0.000590,0.000576,0.000562,0.000547,0.000531,0.000515,0.000498,0.000481,0.000463,0.000445,0.000426,0.000407,0.000388,0.000368,0.000348,0.000328,0.000307,0.000286,0.000265,0.000244,0.000222,0.000200,0.000179,0.000157,0.000135,0.000113,0.000091,0.000069,0.000047,0.000025,0.000003,-0.000019,-0.000040,-0.000062,-0.000083,-0.000104,-0.000125,-0.000145,-0.000165,-0.000185,-0.000205,-0.000224,-0.000243,-0.000262,-0.000280,-0.000298,-0.000315,-0.000332,-0.000348,-0.000364,-0.000379,-0.000394,-0.000408};
