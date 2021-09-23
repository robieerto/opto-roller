#ifndef OPTO_DISTANCE_PARAMS_H
#define OPTO_DISTANCE_PARAMS_H

/* konstanty */

// maximalny pocet vzorkov pre vyrobok
const int ROLLER_BUFFER_SIZE = 4000;
// velkost buffra na priemerovanie
const int MEMORY_BUFFER_SIZE = 1000;

/* nastavitelne */

// maximalna vzdialenost
int DIST_TRESHOLD = 10;
// krok zaznamenavania hodnot
double MEASURE_STEP = 5.0;
// dlzka, na ktorej sa robi priemer
double LENGTH_SCOPE = 1.0;
// kalibrovana hodnota snimaca
double KALIB_OPTO;
// kalibrovany priemer
double KALIB_PRIEMER;
// vzorkov na mm - vzdialenostny snimac
int SAMPLES_PER_MM;

enum Surface { isNothing, isRoller };

#endif //OPTO_DISTANCE_PARAMS_H
