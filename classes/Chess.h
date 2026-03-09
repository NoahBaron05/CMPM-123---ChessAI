#pragma once

#include "Game.h"
#include "Grid.h"
#include "Bitboard.h"
#include "Evaluate.h"

constexpr int pieceSize = 80;
constexpr int WHITE = +1;
constexpr int BLACK = -1;

enum AllBitBoards {
    WHITE_PAWNS,
    WHITE_KNIGHTS,
    WHITE_BISHOPS,
    WHITE_ROOKS,
    WHITE_QUEENS,
    WHITE_KING,
    WHITE_ALL_PIECES,
    BLACK_PAWNS,
    BLACK_KNIGHTS,
    BLACK_BISHOPS,
    BLACK_ROOKS,
    BLACK_QUEENS,
    BLACK_KING,
    BLACK_ALL_PIECES,
    OCCUPANCY,
    EMPTY_SQUARES,
    e_numBitboards
};

class Chess : public Game
{
public:
    Chess();
    ~Chess();

    void setUpBoard() override;

    bool canBitMoveFrom(Bit &bit, BitHolder &src) override;
    bool canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    bool actionForEmptyHolder(BitHolder &holder) override;

    void stopGame() override;

    Player *checkForWinner() override;
    bool checkForDraw() override;

    std::string initialStateString() override;
    std::string stateString() override;
    void setStateString(const std::string &s) override;

    Grid* getGrid() override { return _grid; }

private:
    Bit* PieceForPlayer(const int playerNumber, ChessPiece piece);
    Player* ownerAt(int x, int y) const;
    void FENtoBoard(const std::string& fen);
    char pieceNotation(int x, int y) const;
    void bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst);
    std::vector<BitMove> generateAllMoves();
    void generateKnightMoves(std::vector<BitMove> &moves, std::string &state);
    void generateKingMoves(std::vector<BitMove> &moves, std::string &state);
    void generatePawnMoves(std::vector<BitMove> &moves, std::string &state, int row, int col, int colorAsInt);
    void generateBishopMoves(std::vector<BitMove> &moves, Bitboard piecesBoard, uint64_t occupancy, uint64_t friendlies);
    void generateRookMoves(std::vector<BitMove> &moves, Bitboard piecesBoard, uint64_t occupancy, uint64_t friendlies);
    void generateQueenMoves(std::vector<BitMove> &moves, Bitboard piecesBoard, uint64_t occupancy, uint64_t friendlies);
    void addMoveIfValid(std::string &state, std::vector<BitMove> &moves, int fromRow, int fromCol, int toRow, int toCol);

    Grid* _grid;
    int _currentPlayer;
    std::vector<BitMove> _moves;
    Bitboard _bitboards[e_numBitboards];
    int _bitboardLookup[128];
};