/*
*
* BrogueJS (https://github.com/freethenation)
*
* @freethenation (Richard Klafter)
* @lachesis (Eric Swanson)
*
* Copyright (C) 2019 Richard Klafter
*
*    This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU Affero General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU Affero General Public License for more details.
*
*    You should have received a copy of the GNU Affero General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/

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

// boolean useAscii;
static void javascript_gameLoop(void) {

  // Setup file system
	EM_ASM( FS.mkdir('/brogue'); );

  // Enter the working directory
	char *directory = "/brogue";
	chdir(directory);

  // Mount the directory and load persistent data from browser's IndexedDB
	EM_ASM(
    FS.mount(IDBFS, { autoPersist: true }, '/brogue');
    FS.syncfs(true, function (err) {
      if (err) {
        console.error("Failed to load persistent data: ", err);
      } else {
        console.log("Persistent data loaded sucessfully!");
      }
    })
	);

  // Register a JS handler to queue mouse & keyboard input
  EM_ASM(
    const origin = window.location.origin;
    window.keyOrMouseEvents = [];
    window.addEventListener('message', function(e){
      if (origin !== e.origin) return;
      window.keyOrMouseEvents.push(e.data);
    }, false);
  );

  // // Detect if we need to translate unicode to ascii... grr safari!
  // useAscii = EM_ASM_INT(
  //   return /^((?!chrome|android).)*safari/i.test(navigator.userAgent)?1:0;
  // , 0);

  rogueMain();
}

static void javascript_plotChar(enum displayGlyph glyph,
        short xLoc, short yLoc,
        short foreRed, short foreGreen, short foreBlue,
        short backRed, short backGreen, short backBlue) {

    unsigned int ch = glyphToUnicode(glyph);

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
