Upon first build and launch you have to specify the location of Mage location (the root folder for repo on your disk):
Environment variable MAGE_ENGINE might looks like so after typing "set" command in Command Prompt:
    MAGE_ENGINE=F:\DEV\MAGE\MAGE_GIT\MAGE\

Shortcuts in the editor (can check them and change inside MageEditor\Editors\WorldEditorView.xaml):
    - Build project -> Ctrl+Shift+B (or a Build button)
    - Debug Start (should launch a game with a debugger) -> F5
    - Debug Stop Shift + F5
    - Debug Start Without Debuggiing -> Ctrl + F5
    - Save project -> Ctrl + S

To see (or rather hear) whether or not scripts for the game work, create a test project through editor, add the script and use a code template down below for it:

    #include "example1.h"
    #include <Windows.h>

    namespace TestProject1 {

        REGISTER_SCRIPT(example1);
        void example1::begin_play() {
        
        }

        void example1::update(float dt) {
            int x = 33;
            int y = 55;

            static u32 i = 300;
            if ( (i++ % 100) == 0 ) Beep(i, 100);
        }
    } // namespace TestProject1

Of course you still gotta bind in the editor the script component for as many entities as you wish to.
After building "game" through editor and launching it through editor you should hear Beeps.
