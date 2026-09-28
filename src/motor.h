#pragma once

void  motorSetup();
void  motorUpdate(float dt);  // call every tick; follows settings
float motorTarget();          // -100..100 %, what the current mode asks for
float motorActual();          // -100..100 %, after the ramp
