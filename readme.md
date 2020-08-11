# Psionic

### PuzzleScript Interpreter Open source but Nevertheless Incomplete and not exactly Compliant

An interpreter written in c++ for the [Puzzlescript](https://www.puzzlescript.net/) language. It is not exactly compliant with Puzzlescript though (more on that below).

It is still in "beta" since some features are missing and there are probably some bugs. The API may also be subject to changes.

## Why a c++ Puzzlescript interpreter ?

I find that Puzzlescript is an extremely helpful tool when it comes to prototyping and designing puzzle game. It has however its own set of limitations (which is not necessarily a bad thing but I wanted to overcome some of them).

I wanted to use Puzzlescript in [Godot](https://godotengine.org/) to drive the logic of a game I was making (and avoid rewriting it entirely and Godot's script language) but have more freedom regarding graphics and sounds (among other things). Since I couldn't find any Puzzlescript Godot module, I decided to write my own (which you can find [here](https://github.com/KerDelos/gdpsionic)).

I could have tried to use the Puzzlescript javascript code in the module but I have little knowledge of javascript. I figured it would be less of a headache, more flexible and a more interesting experience to try to write my own interpreter.

Hence Psionic.

## What are the differences between Psionic and Puzzlescript ?

After all, the name says its incomplete and not quite compliant.

#### What's currently missing :
- **most prelude options** ( currently Psionic only supports **author**, **title**, **homepage** and **realtime_interval** )
- **sounds** (the entire section is plainly ignored for now)
- **some commands** ( currently Psionic only supports **win**, **cancel**, **again**)
- **rule groups** (the + symbol)
- **messages**
- **[randomness](https://www.puzzlescript.net/Documentation/randomness.html)**
- **[rigibody](https://www.puzzlescript.net/Documentation/rigidbodies.html)** and loops (which seems to be a related language feature)
- maybe some other things

#### What's currently different :

Psionic executes rules slightly differently than Puzzlescript. However this difference might have a big impact depending on your game.

Puzzlescript takes each rule (each rule group actually) and applies it as much as it can. It starts from the top left cell (not exactly sure about that though) and may loop several time across the level. Each time the rule can be applied it is applied and the level is modified subsequently. Thus, some cells are "prioritized".

Psionic however scan the level once and detect every possible applications of the rule without modifying the level. It then tries to apply all the modifications at the same time. If two (or more) applications conflicts which each other they are canceled (only the conflicting applications, the other ones will be applied and modify the level before the next rule is processed).

#### When will it be complete and will it ever be compliant ?

I don't know when it will be complete (and if it ever will). I add features when I have time and/or need them for my personnal projects. If something you'd love to use is still missing please open up an issue and I'll see what I can do.

As to compliance with Puzzlescript I don't know either. I'd love to have a 100% compliant interpreter. However I also like the different way Psionic handle rules and I'd also like to add extra features to Psionic. Maybe some day it will be possible to switch between a completely compliant Puzzlescript interpreter and Psionic with a prelude option but there are many things to do and I only have so much time.

## How to use Psionic

I'll probably change some of the API soon so I'll wait until then to write more in this section, sorry.

If you want so see an exemple though you can check https://github.com/KerDelos/Psionic-Terminal.


## How to build Psionic

Make sure you have [scons](https://scons.org/) installed and a c++17 compiler.

#### Why scons ?

Since I primarily developped psionic to work withing a Godot module and since Godot is built with scons it seemed more logical to me to use the same build system. I might also support CMake in the future though.

#### Building it on its own

Run `scons platform=<"linux" or "windows">` in the root directory.

You can then run the following executable `build/debug/interpreter`. I won't describe how it works here since I'll soon deprecate it in favor of https://github.com/KerDelos/Psionic-Terminal. Sorry about that !

#### Building it as a library

There's no easy and clean way to build it as a library yet (I'm copying bits of the Psionic sconstruct file in projects that use it) but I'm working on it.

## Contributing

If something is not clear enough in this readme, if you have questions or suggestions, don't hesitate to open up an issue :)

## Thanks

Thanks a lot to [Stephen Lavelle](https://github.com/increpare) for building the awesome thing that puzzlescript is and making it open source !

Thanks a lot to [Bob Nystrom](https://github.com/munificent) for writing https://craftinginterpreters.com/ and making it freely available. I haven't read it entirely yet but it gave me a great starting point when I started to develop Psionic.

Thanks to everybody that use or participate on this project one way or another !


