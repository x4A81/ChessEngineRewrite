#pragma once

#include <array>
#include <string>
#include <cassert>
#include "Globals.hpp"
#include "BitMath.hpp"

constexpr const char* START_POS = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

using std::array, std::string;

enum EncodedCastlingRights : int {
    wking_side = 1,
    wqueen_side,
    bking_side = 4,
    bqueen_side = 8
};

inline constexpr std::array<int, 64> castlingRightsSqEncoder = {
    13, 15, 15, 15, 12, 15, 15, 14,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    7, 15, 15, 15,  3, 15, 15, 11
};

struct IrreversibleState {
    uint8_t castlingRights;
    int epSquare;
    int halfmoveClock;
    Piece capturedPiece;
};

[[nodiscard]] constexpr inline bool isMove(Move move, MoveCode code)  { return (move >> 12) == code; }
[[nodiscard]] constexpr inline Square getFromSq(Move move) { return move & 0b111111; }
[[nodiscard]] constexpr inline Square getToSq(Move move) { return (move >> 6) & 0b111111; }
[[nodiscard]] constexpr inline MoveCode getMoveCode(Move move) { return static_cast<MoveCode>(move >> 12); }
[[nodiscard]] constexpr inline int getFile(Square sq) { return sq & 7; }
[[nodiscard]] constexpr inline int getRank(Square sq) { return sq >> 3; }

class MoveList {
    private:
        array<Move, 256> moves{};
        size_t size = 0;

    public:
        MoveList() : size(0) {
            moves.fill(nullmove);
        }

        void add(Move move) { assert(size < 256); moves[size++] = move; }

        void clear() {
            moves.fill(nullmove);
            size = 0;
        }

        size_t _size() const { return size; }
        auto begin() { return moves.begin(); }
        auto end() { return moves.begin() + size; }
        bool isEmpty() const { return size == 0; }
        const Move& operator[](size_t idx) const {
            return moves[idx];
        }

        Move& operator[](size_t idx) {
            return moves[idx];
        }
};

class Board {
    public:

        IrreversibleState currentState;
        std::array<IrreversibleState, 64> stateHistory;

        // List of the pieces on each square
        array<Piece, 64> pieceList{};

        // Bitboards for each piece
        array<BitBoard, 15> bitBoards{};

        // Colour to move
        Colour sideToMove;
        
        long ply;

        
        Board() {
            loadFen(START_POS);
        }
        
        BitBoard checkers() const;
        bool moreThanOneChecker() const { return popCount(checkers()) > 1; }
        template <GenType type>
        [[gnu::hot]] void generateMoves(MoveList& moveList) const;
        void printBoard() const;

        void loadFen(const string& fen);
        void reset();
        [[gnu::hot]] void makeMove(const Move move);
        [[gnu::hot]] void unmakeMove(const Move move);
        void search();
};