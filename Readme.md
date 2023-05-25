# Repertoire Builder

Automatically generate a chess repertoire by calling the postgres database, which provides similar functionality to the lichess API.

The lichess database fails after too many API calls, makes me wait, etc. Since the database is free, why not just download it and parse it myself?

There is a repertoireBuilder, a parser (to parse the lichess database), and scripts to run the postgres database in docker.

I have to be licensed under the GPL-3 license, as I use the pgn-extract program.

## Usage
You currently have to manually change several values manually and then recompile the program with make. This is very bad, but I am busy and this is the second version of the program. I will fix it soon (in like a week it will be awesome).

You must first download the lichess database from https://database.lichess.org/. You can then change the line in the parse.cpp to be the correct file path (i.e std::string pgnFileInitial = "lichess_db_standard_rated_2013-01.pgn";). 

You also need to start the postgres database with `./build.sh` and run it with `./run.sh`

Then run the parser. It currently stores the parsed file in "/tmp/lichess.pgn", but you can change that if you want. This parsed file is then read, and the result, FEN, and move are extracted from it and stored in the postgres database.

Finally, when the program is run with `./repertoireBuilder`, it will query the postgres database instead of the lichess API. You have to change the FEN value manually.
It outputs to an outputPGN.txt file. 

### Note
The database stores the full FEN, including the en passant information, which lichess sometimes omits. Just note this when querying the database, as the FEN record requires the full FEN.

## Libraries needed

sudo apt install libpq-dev

sudo apt install libpqxx-dev

sudo apt install pgn-extract

## My thought process

1. Have the user input the FEN & get the number of times the position has been reached
2. Query the postgres database for a list of moves. Store this somehow, and traverse it
3. Has the move been played with 1/1000 frequency (from the starting position)? If it has, add it to the tree of nodes. 
4. Repeat this process with every node.
5. Output the tree as a series of pgn files. In order traversal makes the most sense I think.

## Licenses
The LICENSE file contains all of the relevant licenses for the code I used

The "Triple Happy Chess" library uses the MIT license

The "libpqxx" library uses the BSD 3-Clause license

The pgn-extract program uses the GPL-3 license
