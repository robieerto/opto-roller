#ifndef OPTO_DISTANCE_PARAMS_H
#define OPTO_DISTANCE_PARAMS_H

/* konstanty */

// maximalny pocet vzorkov pre vyrobok
const int ROLLER_BUFFER_SIZE = 5000;
// velkost buffra na priemerovanie
const int MEMORY_BUFFER_SIZE = 1000;
// maximalna vzdialenost
int DIST_TRESHOLD = 25;

/* nastavitelne */

// krok zaznamenavania hodnot
double MEASURE_STEP;
// dlzka, na ktorej sa robi priemer
double LENGTH_SCOPE;
// kalibrovana hodnota snimaca
double KALIB_OPTO;
// kalibrovany priemer
double KALIB_PRIEMER;
// kalibracna vzdialenost
double KALIB_DIST;
// vzorkov na mm - vzdialenostny snimac
double SAMPLES_PER_MM;
// pomer medianovych hodnot voci celkovemu poctu
double NUM_MEDIANS_RATE;

#endif //OPTO_DISTANCE_PARAMS_H
