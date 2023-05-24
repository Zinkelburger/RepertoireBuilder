# Repertoire Builder

Automatically generate a chess repertoire by calling the lichess database API. 

It currently fails after too many API calls. I am going to download the lichess database to a hard drive and have it interact with that instead.

## Usage
You currently have to manually change the FEN and then recompile the program with make. This is very bad, but I am busy and this is the first version of the program.

The program is run with `./repertoireBuilder` and outputs to an outputPGN.txt file. 

This is not an elegant solution, and the PGN is not really a PGN, it is some barely-legible UCI moves. But it does work for small cases, so it makes me think the problem is the lichess API blocking me for too many requests. The solution is to download the database.

## Libraries needed

sudo apt install nlohmann-json3-dev

sudo apt install curl

## My thought process

1. Have the user input the FEN & get the number of times the position has been reached
2. Query the lichess database for a list of moves. Store this somehow, and traverse it
3. Has the move been played with 1/1000 frequency (from the starting position)? If it has, add it to the tree of nodes. 
4. Repeat this process with every node.
5. Output the tree as a series of pgn files. In order traversal makes the most sense I think.