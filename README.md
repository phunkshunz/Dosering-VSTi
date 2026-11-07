# KÖLDBÄRAREN

Botten-maskinen i MASKINHALLEN (Phunkshunz AUDIO). Byggs i det här repot (tidigare DOSERING).

- **KURVAN**: rita hur botten duckar efter varje kick. Startas av värdens takt, kicken på sidokedjan eller signalen själv.
- **GILJOTIN**: en ren sub som startar i fas exakt när kicken släpper och kapas när nästa kick kommer (kärnan från DOSERING).
- **TRANSIENT**: ANSLAG och SVANS.
- **SUBDIST**: asymmetrisk mättning av botten under brytpunkten, med mono bas.

Ljudmotorn (`Source/KoldEngine.h`) är ren C++ och testas av `tests/engine_test.cpp` i varje bygge.
