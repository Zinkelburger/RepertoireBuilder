// Copyright Andrew Bernal 2023
#include <pqxx/pqxx>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <cctype>

std::string extractMove(std::ifstream& pgn_file);
void insertToDatabase(pqxx::work& txn, std::string fen, std::string result, std::string move);

int main(int argc, char *argv[]) {
    /*if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " [PGN file] [database connection string]" << std::endl;
        return 1;
    }*/
    std::string pgnFileInitial = "lichess_db_standard_rated_2013-01.pgn";
    std::string databaseConnectionString =
        "host=localhost port=5432 dbname=mydatabase user=myuser password=mypassword";

    std::filesystem::path filePath = "/tmp/lichess.pgn";
    // Check if the file exists
    if (std::filesystem::exists(filePath)) {
        std::cout << "File exists. Skipping parsing.\n";
    } else {
        // Pre-process the PGN file using pgn-extract
        std::string pgn_extract_cmd = "pgn-extract --fencomments -C -N -V ";
        pgn_extract_cmd += pgnFileInitial;
        pgn_extract_cmd += " -o/tmp/lichess.pgn";
        pgn_extract_cmd += " >/dev/null 2>&1";
        int status = system(pgn_extract_cmd.c_str());
        if (status != 0) {
            std::cerr << "Failed to execute pgn-extract command" << std::endl;
            return 1;
        }
    }

    // Open the pre-processed PGN file
    std::ifstream pgn_file("/tmp/lichess.pgn");
    if (!pgn_file.is_open()) {
        std::cerr << "Failed to open /tmp/lichess.pgn" << std::endl;
        return 1;
    }

    // Connect to the PostgreSQL database
    pqxx::connection conn(databaseConnectionString);
    pqxx::work txn(conn);

    double linesRead = 0;
    int multiplier = 0;
    // Parse the PGN data
    std::string line;
    char c;
    // getline until it is no longer the pgn header.
    // Then switch to processing FENs and moves char by char
    while (std::getline(pgn_file, line)) {
        linesRead++;
        std::string result;
        bool moreMoves = true;

        // if c is [, then read the line. Otherwise parse the FEN char by char
        while (std::getline(pgn_file, line), line.find('[') != std::string::npos) {
            linesRead++;
            if (line.find("Result") != std::string::npos) {
                // results can be of varying lengths, but there are always between two ""
                result = line.substr(line.find("\"") + 1);
                result = result.substr(0, result.find("\""));
            }
        }

        // now the header part is done, and we can continue parsing the FENs
        while (moreMoves) {
            std::string fen, move;
            while (pgn_file.get(c), c != '{') { linesRead += 0.02; }
            if (c == '{') {
                while (pgn_file.get(c), c != '}') {
                    linesRead += 0.02;
                    // add everything besides the newlines to the fen
                    if (c != '\n') {
                        fen += c;
                    } else {
                        fen += " ";
                    }
                }
                // first and last char are a <space>
                // fen = fen.substr(1, fen.length()-1);

                // odd edge case where the first char is a newline or something
                // and then the first space is the b KQkq part of the FEN, which is wrong
                std::size_t spaceIndex = fen.find(" ");
                if (spaceIndex < 4) {
                    fen = fen.substr(fen.find(" ") + 1);
                }
                std::size_t lastNonSpace = fen.find_last_not_of(' ');
                fen = fen.substr(0, lastNonSpace + 1);

                move = extractMove(pgn_file);

                // edge case at the end of the game
                if (move == "Event") {
                    move = "";
                    moreMoves = false;
                }

                // printing for debugging
                // std::cout << "FEN:" << fen << ":move:" << move << ":result:" << result << "\n";

                insertToDatabase(txn, fen, result, move);
            }
        }
        if (linesRead > 1500 * multiplier) {
            std::cout << "Read " << linesRead << " lines\n";
            // Commit the transaction. Not too often
            txn.commit();
            multiplier++;
        }
    }

    // Commit the transaction
    txn.commit();

    return 0;
}

std::string extractMove(std::ifstream& pgn_file) {
    std::string move;
    char c;
    // we stop on an alpha character. That char is the start of move (which is also the child)
    // Not the first alphanum, as there is the "1." move number which we do not want
    while (pgn_file.get(c), !isalpha(c)) {}

    move += c;
    // I'm not sure about the edge case of '\n', but I don't want to think about it
    // check if alnum or castling has the - in it (for O-O)
    while (pgn_file.get(c), !pgn_file.eof() && (isalnum(c) || c == '-') && c != '\n') {
        if (c != '\n') {
            move += c;
        }
    }
    return move;
}

void insertToDatabase(pqxx::work& txn, std::string fen, std::string result, std::string move) {
    // Format the move as a PostgreSQL array literal
    move = "{" + move + "}";

    // insert the fen, result, and move into the database
    // only append the child move if it does not already exist in the database
    txn.exec_params(
        "INSERT INTO lichess (fen, white_wins, black_wins, draws, children_moves) "
        "VALUES ($1, $2, $3, $4, $5) "
        "ON CONFLICT (fen) DO UPDATE SET "
        "white_wins = lichess.white_wins + $2, "
        "black_wins = lichess.black_wins + $3, "
        "draws = lichess.draws + $4, "
        "children_moves = CASE WHEN NOT (lichess.children_moves && $5) "
        "THEN array_cat(lichess.children_moves, $5) ELSE lichess.children_moves END",
        fen,
        result == "1-0" ? 1 : 0,
        result == "0-1" ? 1 : 0,
        result == "1/2-1/2" ? 1 : 0,
        move);
}
