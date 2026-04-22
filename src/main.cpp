#include "../include/Board.hpp"
#include "../include/BitMath.hpp"
#include "../include/Globals.hpp"
#include "../include/BitBoards.hpp"
#include <string>
#include <iostream>

std::string squareToString(Square sq) {
    char file = 'a' + getFile(sq);
    char rank = '1' + getRank(sq);
    return std::string() + file + rank;
}

std::string moveToStr(const Move move) {
    Square from_sq = getFromSq(move);
    Square to_sq = getToSq(move);
    MoveCode code = getMoveCode(move);

    std::string result = squareToString(from_sq) + squareToString(to_sq);

    switch (code) {
        case npromo:
        case c_npromo: result += 'n'; break;
        case bpromo:
        case c_bpromo: result += 'b'; break;
        case rpromo:
        case c_rpromo: result += 'r'; break;
        case qpromo:
        case c_qpromo: result += 'q'; break;
        default: break;
    }

    return result;
}

int main() {
    Board board;
    initMagics();
    board.loadFen("2b5/6N1/3P4/3B4/6K1/8/8/5R2 w - - 0 1");
    board.printBoard();

    MoveList list;
    board.generateMoves<GenType::EVASIONS>(list);
    for (Move move : list) {
        std::cout << moveToStr(move) << std::endl;
    }

    return 0;
}