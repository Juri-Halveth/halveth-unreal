#pragma once
// SPDX-License-Identifier: MIT
#include <algorithm>
#include <cmath>

namespace HalvethBody {
// Fictional gameplay physiology. Effective compartments close the mass budget;
// these coefficients are authored controls, not a clinical human simulation.
struct Profile {
    double massKg=68,paceCmS=74,curiosity=.65,reactionSeconds=.18;
    static Profile ForIdentity(int identity){
        if(identity==2)return{82,82,.4,.22};
        if(identity==3)return{61,68,.8,.16};
        return{};
    }
};
struct State {
    double muscleReserve=1,musclePower=1,oxygenSupply=1;
    double heartRateBpm=72,heartPhase=0,breathsPerMinute=12,breathPhase=0;
    double reactionSeconds=.18,tendonStrain=0;
    void advance(const Profile& genes,double speedCmS,double dt){
        if(!std::isfinite(dt)||dt<=0||!std::isfinite(speedCmS))return;
        const double load=std::clamp(std::abs(speedCmS)/genes.paceCmS,0.,1.);
        // Exertion consumes reserve. Standing restores it; neither is a timer
        // animation independent of the body's actual movement.
        muscleReserve=std::clamp(muscleReserve+dt*(.032*(1-load)-.015*load),0.,1.);
        const double oxygenTarget=1-.08*load;
        oxygenSupply=oxygenTarget+(oxygenSupply-oxygenTarget)*std::exp(-dt/2.);
        musclePower=.72+.28*muscleReserve;
        heartRateBpm=72+62*load+12*(1-muscleReserve);
        breathsPerMinute=12+15*load+4*(1-muscleReserve);
        heartPhase=std::fmod(heartPhase+dt*heartRateBpm*6.283185307179586/60.,6.283185307179586);
        breathPhase=std::fmod(breathPhase+dt*breathsPerMinute*6.283185307179586/60.,6.283185307179586);
        reactionSeconds=genes.reactionSeconds+.12*(1-muscleReserve);
        tendonStrain=.03*load*musclePower;
    }
};
struct MassCompartments {
    double skeleton,muscles,organs,fat,skin,blood;
    double total()const{return skeleton+muscles+organs+fat+skin+blood;}
};
inline MassCompartments MassBudget(double kg){return{kg*.12,kg*.4,kg*.14,kg*.2,kg*.06,kg*.08};}
}
