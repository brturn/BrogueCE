#include <emscripten.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
// #include "term.h"
#include <sys/timeb.h>
#include <stdint.h>
#include <signal.h>
#include "platform.h"
#include <stdio.h>
#include <unistd.h>

extern playerCharacter rogue;

boolean useAscii;
void javascript_gameLoop(void) {
  PRINT_DEBUG(0);
  //  EM_ASM({ Module.wasmTable = wasmTable; });

  //register a JS handler to queue mouse & keyboard input
  EM_ASM(
    var origin = window.location.origin;
    window.keyOrMouseEvents = [];
    window.addEventListener('message', function(e){
      if(origin!==e.origin) return;
      window.keyOrMouseEvents.push(e.data);
    }, false);
  );
  //detect if we need to translate unicode to ascii... grr safari!
  useAscii = EM_ASM_INT(
    return /^((?!chrome|android).)*safari/i.test(navigator.userAgent)?1:0;
  , 0);
  rogueMain();
}

void javascript_plotChar(enum displayGlyph ch,
        short xLoc, short yLoc,
        short foreRed, short foreGreen, short foreBlue,
        short backRed, short backGreen, short backBlue) {
  if(useAscii){
    switch (ch) {

        case G_UP_ARROW:          ch = '^'; break;
        case G_DOWN_ARROW:        ch = 'v'; break;
        case G_FLOOR:             ch = '.'; break;
        case G_CHASM:             ch = ':'; break;
        case G_TRAP:              ch = '%'; break;
        case G_FIRE:              ch = '^'; break;
        case G_FOLIAGE:           ch = '&'; break;
        case G_AMULET:            ch = ','; break;
        case G_SCROLL:            ch = '?'; break;
        case G_RING:              ch = '='; break;
        case G_WEAPON:            ch = '('; break;
        case G_GEM:               ch = '+'; break;
        case G_TOTEM:             ch = '0'; break; // zero
        case G_GOOD_MAGIC:        ch = '$'; break;
        case G_BAD_MAGIC:         ch = '+'; break;
        case G_DOORWAY:           ch = '<'; break;
        case G_CHARM:             ch = '7'; break;
        case G_GUARDIAN:          ch = '5'; break;
        case G_WINGED_GUARDIAN:   ch = '5'; break;
        case G_EGG:               ch = 'o'; break;
        case G_BLOODWORT_STALK:   ch = '&'; break;
        case G_FLOOR_ALT:         ch = '.'; break;
        case G_UNICORN:           ch = 'U'; break;
        case G_TURRET:            ch = '*'; break;
        case G_CARPET:            ch = '.'; break;
        case G_STATUE:            ch = '5'; break;
        case G_CRACKED_STATUE:    ch = '5'; break;
        case G_MAGIC_GLYPH:       ch = ':'; break;
        case G_ELECTRIC_CRYSTAL:  ch = '$'; break;
    }
  }
    EM_ASM_({
    if(!window.plotChars){
      setTimeout(function(){
        window.parent.postMessage(Object.values(window.plotChars), '*');
        window.plotChars = null;
      },0);
      window.plotChars = {};
    }
    window.plotChars[($1<<8)+$2] = ([$0,$1,$2,$3,$4,$5,$6,$7,$8]);
    }, ch, xLoc, yLoc, foreRed, foreGreen, foreBlue, backRed, backGreen, backBlue);
}

boolean javascript_pauseForMilliseconds(short milliseconds, PauseBehavior _) {
      // PRINT_DEBUG(1);
    printf("millis %d\n", milliseconds);
    emscripten_sleep(milliseconds);
    // PRINT_DEBUG(1);
  return EM_ASM_INT({
    return Math.min(window.keyOrMouseEvents.length, 1);
  }, 0);
}

//Passing structs from JS is tricky. Instead of doing that JS simply calls the function below with the nextKeyOrMouseEvent
rogueEvent javascript_nextRogueEvent;
static void EMSCRIPTEN_KEEPALIVE javascript_receiveNextKeyOrMouseEvent(enum eventTypes eventType, signed long param1, signed long param2, boolean controlKey, boolean shiftKey) {
  if(eventType != NUMBER_OF_EVENT_TYPES){
    EM_ASM_({console.log($0,$1,$2,$3,$4);},eventType,param1,param2,controlKey,shiftKey);
  }
  javascript_nextRogueEvent.eventType = eventType;
  javascript_nextRogueEvent.param1 = param1;
  javascript_nextRogueEvent.param2 = param2;
  javascript_nextRogueEvent.controlKey = controlKey;
  javascript_nextRogueEvent.shiftKey = shiftKey;
}
static void javascript_nextKeyOrMouseEvent(rogueEvent *returnEvent, boolean textInput, boolean colorsDance) {
//   if (noMenu && rogue.nextGame == NG_NOTHING) rogue.nextGame = NG_NEW_GAME;
  for (;;) {
    if (colorsDance) {
      shuffleTerrainColors(3, true);
      commitDraws();
    }
    EM_ASM_({
      var nxt = window.keyOrMouseEvents.shift();
      if(nxt){
        _javascript_receiveNextKeyOrMouseEvent.apply(null, nxt);
      } else {
        _javascript_receiveNextKeyOrMouseEvent($0, 0, 0, 0, 0);
      }
    }, NUMBER_OF_EVENT_TYPES);
    if(javascript_nextRogueEvent.eventType != NUMBER_OF_EVENT_TYPES){
      returnEvent->eventType = javascript_nextRogueEvent.eventType;
      returnEvent->param1 = javascript_nextRogueEvent.param1;
      returnEvent->param2 = javascript_nextRogueEvent.param2;
      returnEvent->controlKey = javascript_nextRogueEvent.controlKey;
      returnEvent->shiftKey = javascript_nextRogueEvent.shiftKey;
      return;
    }
    emscripten_sleep(50);
  }
}

static void javascript_remap(const char *input_name, const char *output_name) {

}

static boolean modifier_held(int modifier) {
  return 0;
}

struct brogueConsole javascriptConsole = {
  javascript_gameLoop,
  javascript_pauseForMilliseconds,
  javascript_nextKeyOrMouseEvent,
  javascript_plotChar,
  javascript_remap,
  modifier_held,
  NULL,
  NULL,
  NULL
};
