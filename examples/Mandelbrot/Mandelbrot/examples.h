#pragma once
#include <bitloop.h>

SIM_BEG;

using namespace BL;

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

/*inline std::vector<MandelPreset> mandel_presets {
R"(
=============== Home ===============
G6YAAESBdaW-egJD0YvJOjn9d2C8Bt5WgHcO
PPCQkVThM_mFB50lkUQpJnCauU1WVGVWILFb
eG8elpZInAnuDb_z6sWoQjWG4jXXZuazfKp2
F7QMQ0yX_sSotGhzMEy6Cy2QeBzqiZcppPd8
oHN_
====================================
)",
R"(
============= Firework =============
G-AAAOThmKlTZzBhu5iEop5cJ6d_wNYKAw8C
yzEKMOA8hzxuqZn9CxXIkoQWZFEjUqC0i62E
xdWWiduOPjE6CbvSEogk7CHrKOqU4IcRsp2_
cE-oTip2MskN5nV_7WaW5rP6DMa72tgYsbhM
TwaRaXQGY9q7egEkPDbVCk8gXk-KdGQOiMTe
ZSS8IatQo6cYAiNTYgkfyBKx40yMyUbN_Ac=
====================================
)",
R"(
============= Tendrils =============
G00BAMR_pstpZ4XmkWA_FT0TTnQTs9rs9FoB
W9AWWCuhnMIW6IvZu43p0C1RBe0P9sNeQ8HS
7FUGUmpNyWQkWG-s_YrGXWqyUgk22XJNwjRc
oGPb7JSX2BZLXH7idwUO8Gu1nDC50tmBavOB
P2dbQw8DEMedgHDlf-bg1fkcx6pOeYTl4pmq
-8kASDUOoHUxkYDu_iUwGR2qHjmZOvPmqrqA
2VwKuLTb2Tsqu0sGsle8eOvKP2N-zSwa6KPR
BZebdoJOaHrE4xj-8ocz6IMJtnv_1Qq2Z6-x
FOzKXl9kEEpD0lHojCbBbv0xjqyyOjLGkALu
l7HaGNI2MhSE-h8=
====================================
)",
R"(
=========== Julia Island ===========
Gz4BAMRkNqc9K7QHr6k7Fd3E9LHTCtQKKwrT
Eso1yRe09YYtj8OhtJHdX0pRWEsqUFgFlbJc
UJHTxqRNChdTOewpW9JSk7by9LriQgdHX18p
LWm3Bqem7DPVflcVgHi1Glm3XH_44PZmah6l
_szABVjgw4Dem9QGorXZ4NZy8ga6UXtw67Kc
AvnuFXyoZbNwRpkslvNry6Cs8g_R27e2cI_P
Kp6DQemD5vlZhBrXnsbzG6mFeBW3XUzQHUGo
4yDLd_QMQosQzDODBzhGks7viw-8PEp6UTig
g77Hvs9sS3pTOGBv9W0OWZI-FLZV6B8=
====================================
)",
R"(
========== Spiral Spears ===========
G-QAAMQi__yqnzabd3tgQHT4AHVy-gesiRXk
BWRfGEhuY3hdImiV8osakiR0kDhj1oQu0xIO
mR8dL4JNcB273Grtog6EKyStXSatU5ZwvWiy
CYFwN42nQv2yJg_CPu_vl7Y__vBCuunn9KO3
OZuG1cvlSYifyychB6Z7I2bPn12Re7k8n36i
T0MJhOcLJ0l4QZKBCV9IeZl5L12MVhG-P1As
BxM04QcpOMvM_A8=
====================================
)",
R"(
============ Whirlpool =============
GzABAMREXU67Vqs9he0U6An8gBsQ9Y3Tb1Ab
f7DBxvFdp21B3XlEz4jITrbLg34odpjSFMur
iM-UoTP1OtqPcFzF16F4TJPZpo0fBUxT2Gw7
3IBpmU7GZ9pSTInLj31XUIC6VsuJIFc6R9C1
-cCes62hRQBohbsGnJX9mUNV53Mcq37KwikX
z7q6nwyAVOMAvS4mEvC7fwlMRoeqRU5SZ9Vc
VRcINpcCLu129o7K7pKB9IoXG0bln7G6ZhYN
9NHoQsmmndAnNC3M2MBe_nCG_iCm3d38Ee3h
Ki-15HiG6aZNPBGH6U6xEZF_
====================================
)",
R"(
============ Hurricane =============
G9cAAMTCuVJ-bLC8zQGCA4J_nZy2gPEFdsEw
t0tcQrmDdTqtBsHmhxSMxVGA0VJrh6Mockw6
PpWex7GEsbRLfggr9dAiy_Y41hO52QBdjX1k
CDnOMEivUvahvMv40PXK9WegLL_5aj5Lg3mT
etfVTKnv6qysilvcqtblVVTsulo0XynUJsXA
cfmFHNeL9lZCS8slcjk-PPieRcTxhXEodKSm
fw==
====================================
)",
R"(
============= Minibrot =============
GzEBAMRiNqv6bSGZ5RQu_x4TItqJqRucFjA3
2DpYNgBuL4h7Di8llxQ7tnnSL0VKUoYijjN7
krJKSf1Gg8RJZ08H7Eqabt0o40uaIebHrpTv
S1oRu6Lk9de-KwpA3GqVpJ8vX0Lo-mJoL7n2
yMIHtMBDA2ptfxcQtcUCp5qXtlCV0kXXDtMh
kG4eoTelZBJe7z-J6fhYs8hz-iJa69oS_vZa
xLXTyT1Q3V-z4H7paoOw8jsRt-yyiQGaPQje
dpL6jJaFmRjY6z8u0J8kaf_BXUs6UKR8lnSj
KMZxT7lOKOk-mH3j69BIelCknJD5DQ==
====================================
)"
};*/

SIM_END;
