#pragma once
#include <map>

#include <bitloop/utility/text_util.h>

struct MandelPreset
{
    std::string name;
    std::string data;
    MandelPreset(const char* _data)
    {
        data = _data;
        extractName();
    }

    void extractName()
    {
        data = TextUtil::trim_view(data);
        data = TextUtil::dedent_max(data);

        size_t first_nl = data.find_first_of('\n');
        if (first_nl != std::string::npos)
        {
            std::string first_line = data.substr(0, first_nl);
            size_t eq_start = first_line.find_first_not_of('=');
            size_t eq_end = first_line.find_last_not_of('=');

            if (eq_start != std::string::npos &&
                eq_end != std::string::npos &&
                eq_end > eq_start)
            {
                name = first_line.substr(eq_start + 1, eq_end - eq_start - 1);
                name = TextUtil::trim_view(name);
            }
        }
    }
};

inline std::vector<MandelPreset> mandel_presets {
R"(
=============== Home ===============
G6YAAESBdaW+egJD0YvJOjn9d2C8Bt5WgHcO
PPCQkVThM/mFB50lkUQpJnCauU1WVGVWILFb
eG8elpZInAnuDb/z6sWoQjWG4jXXZuazfKp2
F7QMQ0yX/sSotGhzMEy6Cy2QeBzqiZcppPd8
oHN/
====================================
)",
R"(
============= Firework =============
G+AAAGRxTuVj221JIYUDoDDIinVy+gdsrTDw
ILAcowADznPI45aa2b9QgUgSWhAxRSRLaRcr
CYurLRO3HY1n1BJ2pcWTsxL2ELV2aYAfRsh2
/sI9oTqp2CnzejCvm2s3LLPP6jMY72rjLBOL
y/SUIUKjM2RZe1cvgITHpirhCcTrKaHUMVtE
YqODhDfEBFM05KxlZPIs4QORiDUH57xyKfMf
====================================
)",
R"(
============= Tendrils =============
G1ABAGSZzuvPC2vw0rky0U9MdXL6VsCWYInl
EraDBIlJnkOOT8Jq8xdSEOYUChBmHmNK0Yoh
KIxrnzhZVKasYZLCcrXxZaAprCBsPBn4QlNY
X+sOp4TsMyW/VAkiebUaWV2uP3y07c3UPUr9
mUONaAl+LCLfu9QGSWuzwVtL5R3yRu1hW5fl
FDHfvaI91LJZVKNMFpfza8thmeUfpLdvbVEf
n1V8DgalDzbPzyKyce3pjN9ILciruO3iBLsj
JOw4yNo79kiDRYDumcEH2hhQOL9/NRQuEA4Y
hReEQ8zjTFvmW+5rZSm88zYQRholtNZWAh+m
jdLaKiO09bj6Aw==
====================================
)",
R"(
=========== Julia Island ===========
GzwBAMRCu6p+28MkOe3bebCDqMPTFagVtoDi
xHJM8gva3YlrHRwdHpjuL6UoLE0qUFjYQigP
WrTkuMmJZDGFy75wTFqOq4NVKeFHrm+YuFvC
qSn7TJXfVQ3AeLUaWa9cfwTg9maqH6X+TMMD
2MCHAbnXqQ2M1maDW8vNa8hG7cGty3IK5LtX
8KGWzcIdZbJYzq8tjbLIP4zevrWFd3xW8RwM
Sh80z88ixLj21H7QSC2MV3HbxQTdEQxxHGT5
jp5GaBGCfmbwAMfIpPP77giXV2HSi8KWtAOf
g4DZMelNYcveGjgcUiZ9KOwIxh8=
====================================
)",
R"(
========== Spiral Spears ===========
G98AAMQivap+2mgvxlsdRDc47UFtHmg92OdB
aB5k5xHpWcSMe/NFBJvxyMMmkUjhURA1HuOa
TzJZJKmm3qPLIEUnVlNhsmhoPNY7zDR57AZx
KsTLKDxI2nm072il2sOQmN54Nh7F/qxhdi/L
qSR9lieJldXCRmqenwWJuyznjSd6w/TA43zh
xHhcYDOTeLxgC0w0DKZblibzeH+w0chUTYXH
B7apa0REfw==
====================================
)",
R"(
============ Whirlpool =============
Gy4BAERF2a4/7wopYj5EPzF94/QbZED+wcZt
HHeetgV55xgWkyCIe2GXL1IIuwQFhNWp6hUU
6YLxRCcJF129Qb96BMtdDgTctLL1bbp9gl05
hbyCE8LIPlP2XdWQ5tVqZH3l+iPAYHsztY9S
f2bpI4OGnyDp2tvUhqa12fDW8uYtXY3aI9i6
LKdkvntl8FDLZukdZbJczq8ty7LmH6a3b23p
Oz6rfA4GpQ+b52eROq49rT/QSC3Mq7jtcsLu
iEaPg2zwzp5laBGifWb4YDAGwfml8AkugJc4
esXlCQne2tSj6hJ8EA6p6h8=
====================================
)",
R"(
============ Hurricane =============
G9UAAMTCuVJ+bLC8zQGCA4J/nZy2gPElYbtg
AJbEJZbbIZmw2e03P6RgLI4CjJZaOxxFkWMy
0WnreRxLGEu75JuwUjq+G7b2+2Y/tJ3Y91MY
cpxhkF6lyoPyLuND1yvXn4Gy/Oar+SwN5k3q
XVczpb6rs7IqbnGrWpdXUbHratF8oTYpBo7L
J+S4XpR3J7S0XCKX48OD71lEHF8Yh0JHavoH
====================================
)",
R"(
============= Minibrot =============
GzEBAMRiNqv6bSGZ5ZQj/x4TItqJqRucFjA3
2DpYNgBuL4h7Di8llxQ7tnnSLxklKUOG48ye
pKxSUr/RIHHS2dMBu5KmW1cq9CXNEPNjV8r3
Ja2IXVHy+mvfFQUgbrVK0s+XLyF0fTG0l1x7
ZOEDWuChAbW2vwuI2mKBU81LW6hK6aJrh+kQ
SDeP0JtSMgmv95/EdHysWeQ5fRGtdW0Jf3st
4trp5B6o7q9ZcL90tUFY+Z2IW3bZxADNHgRv
O0l9RssimkSw139coD9J0v6Du5Z0IKN8lnQj
E+O4p1wnlHQfzH7k6zCS9CCjnJD5DQ==
====================================
)"
};
