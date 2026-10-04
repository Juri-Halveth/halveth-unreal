-- SPDX-License-Identifier: MIT
-- Native upper-body idle variation. AI, locomotion and scripted actions remain
-- under their existing controllers; gesture priority is below movement/combat.
local self=require('openmw.self')
local anim=require('openmw.animation')
local core=require('openmw.core')
local I=require('openmw.interfaces')
local candidates={'idle2','idle3','idle4','idle5','idle6','idle7','idle8','idle9'}
local initialized=false
local available={}
local phase=0
local wait=2
local selected=nil
local lastTime=-1
local samples=0
local seed=0
for i=1,#self.recordId do seed=(seed*33+self.recordId:byte(i))%997 end
wait=1.8+(seed%13)*.17
local function idle(name)return name and name:sub(1,4)=='idle' end
return {engineHandlers={onUpdate=function(dt)
    if dt<=0 or not self:isActive() then return end
    if not initialized then
        if not anim.hasAnimation(self) then return end
        for _,name in ipairs(candidates)do if anim.hasGroup(self,name)then available[#available+1]=name end end
        initialized=true
        print('VEYRA_MOTION|READY|record='..self.recordId..'|native_variants='..#available..'|AI_preserved=true')
    end
    if #available==0 then return end
    if selected and anim.isPlaying(self,selected)then
        local t=anim.getCurrentTime(self,selected)
        if t>lastTime+.20 then
            lastTime=t;samples=samples+1
            if self.recordId=='heidmir'then print('VEYRA_MOTION|SAMPLE|record=heidmir|group='..selected..'|time='..t..'|sample='..samples)end
        end
        return
    end
    local lower=anim.getActiveGroup(self,anim.BONE_GROUP.LowerBody)
    local torso=anim.getActiveGroup(self,anim.BONE_GROUP.Torso)
    if not idle(lower) or (torso and not idle(torso))then wait=math.max(wait,.8);return end
    wait=wait-dt
    if wait>0 then return end
    phase=phase+1
    selected=available[(seed+phase-1)%#available+1]
    I.AnimationController.playBlendedAnimation(selected,{
        blendMask=anim.BLEND_MASK.UpperBody,
        priority=anim.PRIORITY.WeaponLowerBody,
        loops=0,autoDisable=true,speed=.82+(seed%7)*.035,
    })
    lastTime=-1;wait=2.2+(seed%11)*.13
    print('VEYRA_MOTION|GESTURE|record='..self.recordId..'|group='..selected..'|time='..core.getSimulationTime()..'|upper_body_only=true')
end}}
