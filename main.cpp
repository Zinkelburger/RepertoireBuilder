// Copyright Andrew Bernal 2023
#include <pqxx/pqxx>
#include <algorithm>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include "thc.h"

class chessNode;
// builds the tree of chess nodes to put in the PGN file
void buildTree(pqxx::work& txn, chessNode* node,
const std::string& FEN, bool whiteToMove, int totalGamesFromStartingPosition);
// returns the move with the highest win rate from the childrenMoves
std::string getBestWhiteMove(pqxx::work& txn,
const std::vector<std::string>& childrenMoves, const std::string& FEN);
void traverseTree(chessNode* root, std::string pgn, std::ofstream& outputFile);
int getStartingTotalNumGames(pqxx::work& txn, std::string FEN);

class chessNode {
 public:
    chessNode(int wInp, int bInp, int dInp, std::string moveInp) {
        whiteWin = wInp;
        blackWin = bInp;
        drawn = dInp;
        UCImove = moveInp;
    }
    chessNode* getChild(int index) {
        return children[index];
    }
    void addChild(chessNode* toAdd) {
        children.push_back(toAdd);
    }
    std::string getMove() {
        return UCImove;
    }
    int getNumChildren() {
        return children.size();
    }

 private:
    std::string UCImove;
    int whiteWin;
    int blackWin;
    int drawn;
    std::vector<chessNode*> children;
};

int main(void) {
    // take configurations from configuration.txt
    std::string FEN;
    std::string databaseConnectionString;
    std::ifstream configFile("configuration.txt");
    std::string line;
    while (std::getline(configFile, line)) {
        if (line.find("FEN") != std::string::npos) {
            FEN = line.substr(line.find("=") + 1);
        } else if (line.find("databaseConnectionString") != std::string::npos) {
            databaseConnectionString = line.substr(line.find("=") + 1);
        }
    }
    // Connect to the PostgreSQL database
    pqxx::connection conn(databaseConnectionString);
    pqxx::work txn(conn);

    bool whiteToMove = false;
    chessNode root(0, 0, 0, "");

    int totalGamesFromStart = getStartingTotalNumGames(txn, FEN);
    buildTree(txn, &root, FEN, whiteToMove, totalGamesFromStart);

    std::ofstream ofs("outputPGN.txt");
    traverseTree(&root, "", ofs);
    ofs.close();

    return 0;
}

std::string getBestWhiteMove(pqxx::work& txn, const std::vector<std::string>& childrenMoves,
const std::string& FEN) {
    std::cout << "Children moves: ";
    for (const auto& move : childrenMoves) {
        std::cout << move << '|';
    }
    std::cout << '\n';
    // Select the highest win rate white move from the top 3 most played moves
    std::vector<std::string> sortedMoves = childrenMoves;
    std::sort(sortedMoves.begin(), sortedMoves.end(),
    [&](const std::string& a, const std::string& b) {
        // Initialize the chessboard with the given FEN
        thc::ChessRules cr;
        cr.Forsyth(FEN.c_str());

        // Make move a on the chessboard
        thc::Move mv;
        mv.NaturalIn(&cr, a.c_str());
        cr.PlayMove(mv);

        // Generate the updated FEN for move a
        std::string updatedFenA = cr.ForsythPublish();

        // Query the database for data for move a
        pqxx::result aResult = txn.exec_params(
            "SELECT white_wins, black_wins, draws FROM lichess WHERE fen = $1",
            updatedFenA);

        // Reset the chessboard to the original FEN
        cr.Forsyth(FEN.c_str());

        // Make move b on the chessboard
        mv.NaturalIn(&cr, b.c_str());
        cr.PlayMove(mv);

        // Generate the updated FEN for move b
        std::string updatedFenB = cr.ForsythPublish();

        // Query the database for data for move b
        pqxx::result bResult = txn.exec_params(
            "SELECT white_wins, black_wins, draws FROM lichess WHERE fen = $1",
            updatedFenB);

        int aTotal = aResult[0][0].as<int>() + aResult[0][1].as<int>() +
                    aResult[0][2].as<int>();
        int bTotal = bResult[0][0].as<int>() + bResult[0][1].as<int>() +
                    bResult[0][2].as<int>();

        return aTotal > bTotal;
    });
    sortedMoves.resize(std::min<size_t>(3, sortedMoves.size()));

    auto it = std::max_element(sortedMoves.begin(), sortedMoves.end(),
    [&](const std::string& a, const std::string& b) {
        // Initialize the chessboard with the given FEN
        thc::ChessRules cr;
        cr.Forsyth(FEN.c_str());

        // Make move a on the chessboard
        thc::Move mv;
        mv.NaturalIn(&cr, a.c_str());
        cr.PlayMove(mv);

        // Generate the updated FEN for move a
        std::string updatedFenA = cr.ForsythPublish();

        // Query the database for data for move a
        pqxx::result aResult = txn.exec_params(
            "SELECT white_wins, black_wins, draws FROM lichess WHERE fen = $1",
            updatedFenA);

        // Reset the chessboard to the original FEN
        cr.Forsyth(FEN.c_str());

        // Make move b on the chessboard
        mv.NaturalIn(&cr, b.c_str());
        cr.PlayMove(mv);

        // Generate the updated FEN for move b
        std::string updatedFenB = cr.ForsythPublish();

        // Query the database for data for move b
        pqxx::result bResult = txn.exec_params(
            "SELECT white_wins, black_wins, draws FROM lichess WHERE fen = $1",
            updatedFenB);

        int aTotal = aResult[0][0].as<int>() + aResult[0][1].as<int>() +
                    aResult[0][2].as<int>();
        int bTotal = bResult[0][0].as<int>() + bResult[0][1].as<int>() +
                    bResult[0][2].as<int>();

        double aWinRate =
            aTotal == 0 ? 0 : static_cast<double>(aResult[0][0].as<int>()) / aTotal;

        double bWinRate =
            bTotal == 0 ? 0 : static_cast<double>(bResult[0][0].as<int>()) / bTotal;

        return aWinRate < bWinRate;
    });
    return *it;
}

void buildTree(pqxx::work& txn, chessNode* node, const std::string& FEN,
bool whiteToMove, int totalGamesFromStartingPosition) {
    // Query the database for data for the given FEN
    pqxx::result result = txn.exec_params("SELECT white_wins, black_wins, draws, "
    "children_moves FROM lichess WHERE fen = $1", FEN);

    // Check if a row was returned
    if (result.size() == 0) {
        // No data was found for the given FEN
        return;
    }

    // Get the data from the result
    int whiteWins = result[0][0].as<int>();
    int blackWins = result[0][1].as<int>();
    int draws = result[0][2].as<int>();
    // Manually parse the children_moves array
    std::string childrenMovesStr = result[0][3].as<std::string>();
    std::vector<std::string> childrenMoves;
    std::stringstream ss(childrenMovesStr.substr(1, childrenMovesStr.size() - 2));
    std::string tempMove;
    while (std::getline(ss, tempMove, ',')) {
        childrenMoves.push_back(tempMove);
    }

    if (whiteToMove && childrenMoves.size() > 0) {
        const std::string& move = getBestWhiteMove(txn, childrenMoves, FEN);
        std::cout << move << "\n";

        chessNode* child = new chessNode(whiteWins, blackWins, draws, move);
        node->addChild(child);

        // Initialize the chessboard with the given FEN
        thc::ChessRules cr;
        cr.Forsyth(FEN.c_str());

        // Make the move on the chessboard
        thc::Move mv;
        mv.NaturalIn(&cr, move.c_str());
        cr.PlayMove(mv);

        // Generate the updated FEN
        std::string updatedFen = cr.ForsythPublish();

        buildTree(txn, child, updatedFen, !whiteToMove, totalGamesFromStartingPosition);
    } else {
        // all of the black moves with 1/1000 frequency of being played in the starting position
        for (const auto& move : childrenMoves) {
            // Generate the updated FEN
            thc::ChessRules cr;
            cr.Forsyth(FEN.c_str());
            thc::Move mv;
            mv.NaturalIn(&cr, move.c_str());
            cr.PlayMove(mv);
            std::string updatedFen = cr.ForsythPublish();

            pqxx::result moveResult = txn.exec_params(
                "SELECT white_wins, black_wins, draws FROM lichess WHERE fen = $1", updatedFen);

            int whiteWins = moveResult[0][0].as<int>();
            int blackWins = moveResult[0][1].as<int>();
            int draws = moveResult[0][2].as<int>();
            int totalChildGames = whiteWins + blackWins + draws;

            double probability = static_cast<double>(totalChildGames) /
                totalGamesFromStartingPosition;

            // 1/200 = 0.005, which is greater than 0.001.
            // If there were only 200 games from a positon, the probability will never be 0.01
            if (probability > 0.001 && totalChildGames > 5) {
                chessNode* child = new chessNode(whiteWins, blackWins, draws, move);
                node->addChild(child);

                buildTree(txn, child, updatedFen, !whiteToMove,
                    totalGamesFromStartingPosition);
            }
        }
    }
}

int getStartingTotalNumGames(pqxx::work& txn, std::string FEN) {
    // Query the database for data for the given FEN
    pqxx::result result = txn.exec_params(
        "SELECT white_wins, black_wins, draws FROM lichess WHERE fen = $1", FEN);

    // Check if a row was returned
    if (result.size() == 0) {
        // No data was found for the given FEN
        return 1;
    }

    // Get the data from the result
    int whiteWins = result[0][0].as<int>();
    int blackWins = result[0][1].as<int>();
    int draws = result[0][2].as<int>();
    int totalGamesFromStartingPosition = whiteWins + blackWins + draws;
    return totalGamesFromStartingPosition;
}

void traverseTree(chessNode* root, std::string pgn, std::ofstream& outputFile) {
    pgn += root->getMove();
    pgn += " ";
    if (root->getNumChildren() <= 0) {
        pgn += "\n";
        outputFile << pgn;
    }
    for (int i = 0; i < root->getNumChildren(); i++) {
        traverseTree(root->getChild(i), pgn, outputFile);
    }
}

