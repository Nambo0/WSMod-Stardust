#include "include/mkb.h"
#include "include/validate.h"

namespace achievement {

//Makes ball.whatever easier to use
mkb::Ball& ball = mkb::balls[mkb::curr_player_idx];

static void claim_achievement(u16 achievement_id){
    // This is for testing and will get replaced later
    if(ball.banana_count != achievement_id){
        ball.banana_count = achievement_id;
    }
}

void tick() {

  // Detect stage challenges
  if(validate::currently_valid){
  switch (mkb::g_current_stage_id) {
  // DOUBLE TAKE | 1-8 Double Time  -  Clear the stunt goal at both spinning speeds (ID: 1)
  case 4:
  {
  
  break;
  }
  // UP, UP, AND AWAY | 2-6 Liftoff  -  Soar higher than the highest cloud onstage (ID: 2)
  case 12:
  {
    if((mkb::sub_mode == mkb::SMD_GAME_PLAY_INIT ||
    mkb::sub_mode == mkb::SMD_GAME_PLAY_MAIN) &&
    ball.pos.y >= 360){
        claim_achievement(2);
    }
  break;
  }
  // DEFUSED | 3-10 Detonation  -  Clear the blue goal without activating the bomb switch (ID: 3)
  case 17:
  {
  
  break;
  }
  // I WANNA BE THE STUNT GOAL | 4-9 Avoidance  -  Clear the stunt goal without hitting any bouncepads (ID: 4)
  case 26:
  {
  
  break;
  }
  // LOCKED DOORS | 5-6 Door Dash  -  Clear the blue goal without opening any doors (ID: 5)
  case 33:
  {
  
  break;
  }
  // MONOCHROMATIC | 6-1 Recolor  -  Clear any goal without entering a color-changing portal (ID: 6)
  case 38:
  {
  
  break;
  }
  // TARGET MASTER | 7-10 Break the Targets  -  Break all 8 targets and finish with time bonus (150s) (ID: 7)
  case 48:
  {
  
  break;
  }
  // JUST HAD TO CHECK | 8-5 Raise the Roof  -  Click 3 different stunt goal buttons (across any number of attempts) (ID: 8)
  case 53:
  {
  
  break;
  }
  // FLIP WIZARD | 9-3 Flip Switches  -  Clear the stage without flipping the switches once (ID: 9)
  case 61:
  {
  
  break;
  }
  // STARSTRUCK | 10-10 Impact  -  Finish in the stunt goal after it shoots into the sky (ID: 10)
  case 350:
  {
  
  break;
  }
  }
  }
}

void init(){}

} // namespace achievement