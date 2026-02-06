#include <emscripten.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>
#include <stdint.h>
#include <signal.h>
#include "platform.h"
#include <stdio.h>
#include <unistd.h>

extern playerCharacter rogue;

boolean useAscii;
static void javascript_gameLoop(void) {

  // register a JS handler to queue mouse & keyboard input
  EM_ASM(
    var origin = window.location.origin;
    window.keyOrMouseEvents = [];
    window.addEventListener('message', function(e){
      if(origin!==e.origin) return;
      window.keyOrMouseEvents.push(e.data);
    }, false);
  );

  // detect if we need to translate unicode to ascii... grr safari!
  useAscii = EM_ASM_INT(
    return /^((?!chrome|android).)*safari/i.test(navigator.userAgent)?1:0;
  , 0);
  rogueMain();
}


static char glyphToAscii(enum displayGlyph glyph) {
    unsigned int ch;

    switch (glyph) {
        case G_UP_ARROW: return '^';
        case G_DOWN_ARROW: return 'v';
        case G_FLOOR: return '.';
        case G_CHASM: return ':';
        case G_TRAP: return '%';
        case G_FIRE: return '^';
        case G_FOLIAGE: return '&';
        case G_AMULET: return ',';
        case G_SCROLL: return '?';
        case G_RING: return '=';
        case G_WEAPON: return '(';
        case G_GEM: return '+';
        case G_TOTEM: return '0'; // zero
        case G_GOOD_MAGIC: return '$';
        case G_BAD_MAGIC: return '+';
        case G_DOORWAY: return '<';
        case G_CHARM: return '7';
        case G_GUARDIAN: return '5';
        case G_WINGED_GUARDIAN: return '5';
        case G_EGG: return 'o';
        case G_BLOODWORT_STALK: return '&';
        case G_FLOOR_ALT: return '.';
        case G_UNICORN: return 'U';
        case G_TURRET: return '*';
        case G_CARPET: return '.';
        case G_STATUE: return '5';
        case G_CRACKED_STATUE: return '5';
        case G_MAGIC_GLYPH: return ':';
        case G_ELECTRIC_CRYSTAL: return '$';

        default:
            ch = glyphToUnicode(glyph);
            brogueAssert(ch < 0x80); // assert ascii
            return ch;
    }
}

static void javascript_plotChar(enum displayGlyph ch,
        short xLoc, short yLoc,
        short foreRed, short foreGreen, short foreBlue,
        short backRed, short backGreen, short backBlue) {

    ch = glyphToAscii(ch);

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

static boolean javascript_pauseForMilliseconds(short milliseconds, PauseBehavior _) {
  emscripten_sleep(milliseconds);
  return EM_ASM_INT({
    return Math.min(window.keyOrMouseEvents.length, 1);
  }, 0);
}

// Passing structs from JS is tricky. Instead of doing that JS simply calls the function below with the nextKeyOrMouseEvent
rogueEvent javascript_nextRogueEvent;
void EMSCRIPTEN_KEEPALIVE javascript_receiveNextKeyOrMouseEvent(enum eventTypes eventType, signed long param1, signed long param2, boolean controlKey, boolean shiftKey) {
  // if(eventType != NUMBER_OF_EVENT_TYPES){
  //   EM_ASM_({console.log($0,$1,$2,$3,$4);},eventType,param1,param2,controlKey,shiftKey);
  // }
  javascript_nextRogueEvent.eventType = eventType;
  javascript_nextRogueEvent.param1 = param1;
  javascript_nextRogueEvent.param2 = param2;
  javascript_nextRogueEvent.controlKey = controlKey;
  javascript_nextRogueEvent.shiftKey = shiftKey;
}

void javascript_nextKeyOrMouseEvent(rogueEvent *returnEvent, boolean textInput, boolean colorsDance) {
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

static boolean javascript_modifier_held(int modifier) {
  return 0;
}

struct brogueConsole javascriptConsole = {
  javascript_gameLoop,
  javascript_pauseForMilliseconds,
  javascript_nextKeyOrMouseEvent,
  javascript_plotChar,
  javascript_remap,
  javascript_modifier_held,
  NULL,
  NULL,
  NULL
};
