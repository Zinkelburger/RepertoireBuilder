# Repertoire Builder
Automatically generate a chess repertoire by calling the postgres database, which provides similar functionality to the lichess API.

The lichess database fails after too many API calls, makes me wait, etc. Since the database is free, why not just download it and parse it myself?

There is a repertoireBuilder, a parser (to parse the lichess database), and scripts to run the postgres database in docker.

I have to be licensed under the GPL-3 license, as I use the pgn-extract program.

## Usage
Use the configuration.txt file to tell the program the FEN you want to start with, where your lichess database is, and where it should store the files it parses.

You must first download the lichess database from https://database.lichess.org/.

You also need to start the postgres database with `./build.sh` and run it with `./run.sh`

Then run the parser. This parsed file is then read, and the result, FEN, and move are extracted from it and stored in the postgres database. Both the preprocessed file (which has pgn-extract run on it) and the postprocessed file (which is similar to the database) remain on the machine. This is probably not a good solution, as there is a lot of data here.

Finally, when the program is run with `./repertoireBuilder`, it will query the postgres database instead of the lichess API. It outputs to an outputPGN.txt file. 

### Note
The database stores the full FEN, including the en passant information, which lichess sometimes omits.

## Installations needed
sudo apt install libpqxx-dev

sudo apt install pgn-extract

## My thought process

1. Have the user input the FEN & get the number of times the position has been reached
2. Query the postgres database for a list of moves. Store this and traverse it
3. Has the move been played with 1/1000 frequency (from the starting position)? If it has, add it to the tree of nodes. 
4. Repeat this process with every node.
5. Output the tree as a series of pgn files.

## Licenses
The LICENSE file contains all of the relevant licenses for the code I used

The "Triple Happy Chess" library uses the MIT license

The "libpqxx" library uses the BSD 3-Clause license

The pgn-extract program uses the GPL-3 license
