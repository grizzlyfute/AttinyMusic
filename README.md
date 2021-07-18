# General purpose
A simple button doing some music when pressed.

The music is randomly selected in a collection of 8 bits musics.
The music is generated throught an attiny, connected to a buzzer

The according is standard wind orchestra, with 442 Hz as A note.
If you want 440 Hz, just change the note table.

# Schematic and doc
See doc repository.
Use atmel atmega85+ datasheet

Button use internal pullup.

# Compilation and import
Do 'make' for compilation
Do 'make upload' to upload throught your avrdude (arduino) programmer

# Available title
- Age of empire I
- Age of empire II
- Age of empire II (2)
- Angry bird
- Bubble booble
- Bump N Jump
- Burger time
- Buzz bomber
- Donkey kong
- En er mundo
- Final fantaisy 7
- Frogger
- Fzero maximum velocity
- Knight quest map theme
- Mario (super)
- Mario (underworld)
- Morrowind - The elders scrolls III (morrowind) theme
- Mountain king mini
- Mule theme
- Pacman
- Popcorn
- Portal - Still alive
- Rescue and dales rescues ranger happy cat
- Snow white and the seven darfs
- Sonic the Hedgdog 2 title
- Sonic the Hedgdog 3 title
- Star trek the motion picture
- Tetris A
- Tetris B
- The monkey island
- The monkey island scummbar
- Tomb raider
- Zelda

# Requierements
- avr-gcc
- make
- avrdude programmer
