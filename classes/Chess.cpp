#include "Chess.h"
#include "MagicBitboards.h"
#include <limits>
#include <cmath>

Chess::Chess()
{
    _grid = new Grid(8, 8);

    initMagicBitboards();

    for (int i = 0; i < 128; i++) {
        _bitboardLookup[i] = 0;
    }

    _bitboardLookup['P'] = WHITE_PAWNS;
    _bitboardLookup['N'] = WHITE_KNIGHTS;
    _bitboardLookup['B'] = WHITE_BISHOPS;
    _bitboardLookup['R'] = WHITE_ROOKS;
    _bitboardLookup['Q'] = WHITE_QUEENS;
    _bitboardLookup['K'] = WHITE_KING;
    _bitboardLookup['p'] = BLACK_PAWNS;
    _bitboardLookup['n'] = BLACK_KNIGHTS;
    _bitboardLookup['b'] = BLACK_BISHOPS;
    _bitboardLookup['r'] = BLACK_ROOKS;
    _bitboardLookup['q'] = BLACK_QUEENS;
    _bitboardLookup['k'] = BLACK_KING;
}

Chess::~Chess()
{
    delete _grid;
}

char Chess::pieceNotation(int x, int y) const
{
    const char *wpieces = { "0PNBRQK" };
    const char *bpieces = { "0pnbrqk" };
    Bit *bit = _grid->getSquare(x, y)->bit();
    char notation = '0';
    if (bit) {
        notation = bit->gameTag() < 128 ? wpieces[bit->gameTag()] : bpieces[bit->gameTag()-128];
    }
    return notation;
}

Bit* Chess::PieceForPlayer(const int playerNumber, ChessPiece piece)
{
    const char* pieces[] = { "pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png" };

    Bit* bit = new Bit();
    // should possibly be cached from player class?
    const char* pieceName = pieces[piece - 1];
    std::string spritePath = std::string("") + (playerNumber == 0 ? "w_" : "b_") + pieceName;
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);

    bit->setGameTag(playerNumber == 0 ? piece : piece + 128);

    return bit;
}

void Chess::setUpBoard()
{
    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");

    _currentPlayer = WHITE;
    _moves = generateAllMoves();

    startGame();
}

void Chess::FENtoBoard(const std::string& fen) {
    // convert a FEN string to a board
    // FEN is a space delimited string with 6 fields
    // 1: piece placement (from white's perspective)
    // NOT PART OF THIS ASSIGNMENT BUT OTHER THINGS THAT CAN BE IN A FEN STRING
    // ARE BELOW
    // 2: active color (W or B)
    // 3: castling availability (KQkq or -)
    // 4: en passant target square (in algebraic notation, or -)
    // 5: halfmove clock (number of halfmoves since the last capture or pawn advance)

    _grid->forEachSquare([](ChessSquare* square, int x, int y){
        square->destroyBit();
    });

    int file = 0;
    int rank = 7; // FEN starts with rank 8, which is index 7 in our grid, because we index from 0 and start with white's perspective

    for (char c: fen){
        // Checks for end of FEN string
        if (c == ' ') {
            break;
        }

        // Checks for end of rank, then proceeds to next rank, and resets file
        if (c == '/'){
            file = 0;
            rank--;
            continue;
        }

        // Checks for number, which indicates how many empty squares there are, and moves the file over by that amount
        if (c >= '1' && c <= '8') {
            file += c - '0';
            continue;
        }

        // Finds what kind of piece it is, and what color it is, then creates the appropriate piece and places it on the board
        bool isWhite = (isupper(c));
        char playerNumber = isWhite ? 0 : 1;

        ChessPiece piece;
        switch(tolower(c)) {
            case 'p' : piece = Pawn; break;
            case 'n' : piece = Knight; break;
            case 'b' : piece = Bishop; break;
            case 'r' : piece = Rook; break;
            case 'q' : piece = Queen; break;
            case 'k' : piece = King; break;
        }

        // create the piece and place it on the board
        ChessSquare* square = _grid->getSquare(file, rank);
        Bit* bit = PieceForPlayer(playerNumber, piece);
        square->setBit(bit);
        bit->setPosition(square->getPosition());
        file++;
    }
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    // need to implement friendly/unfriendly in bit so for now this hack
    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;
    int pieceColor = bit.gameTag() & 128;
    if (pieceColor != currentPlayer) return false;

    ChessSquare* square = (ChessSquare *)&src;
    int squareIndex = square->getSquareIndex();
    for (auto move : _moves) {
        if (move.from == squareIndex) {
            return true;
        }
    }

    return false;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    ChessSquare* squareSrc = (ChessSquare *)&src;
    ChessSquare* squareDst = (ChessSquare *)&dst;
    
    int squareIndexSrc = squareSrc->getSquareIndex();
    int squareIndexDst = squareDst->getSquareIndex();
    for (auto move : _moves) {
        if (move.from == squareIndexSrc && move.to == squareIndexDst) {
            return true;
        }
    }
    return false;
}

void Chess::bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    _currentPlayer = (_currentPlayer == WHITE) ? BLACK : WHITE;
    _moves = generateAllMoves();
    endTurn();
}

void Chess::stopGame()
{
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
}

Player* Chess::ownerAt(int x, int y) const
{
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return nullptr;
    }

    auto square = _grid->getSquare(x, y);
    if (!square || !square->bit()) {
        return nullptr;
    }
    return square->bit()->getOwner();
}

Player* Chess::checkForWinner()
{
    return nullptr;
}

bool Chess::checkForDraw()
{
    return false;
}

std::string Chess::initialStateString()
{
    return stateString();
}

std::string Chess::stateString()
{
    std::string s;
    s.reserve(64);
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
            s += pieceNotation( x, y );
        }
    );
    return s;}

void Chess::setStateString(const std::string &s)
{
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        int index = y * 8 + x;
        char playerNumber = s[index] - '0';
        if (playerNumber) {
            square->setBit(PieceForPlayer(playerNumber - 1, Pawn));
        } else {
            square->setBit(nullptr);
        }
    });
}

std::vector<BitMove> Chess::generateAllMoves(){
    std::vector<BitMove> moves;
    moves.reserve(32);
    std::string state = stateString();

    for (int i = 0; i < e_numBitboards; i++) {
        _bitboards[i] = 0;
    }

    for (int i = 0; i < 64; i++){
        int bitIndex = _bitboardLookup[state[i]];
        _bitboards[bitIndex] |= 1ULL << i;

        if (state[i] != '0'){
            _bitboards[OCCUPANCY] |= 1ULL << i;
            _bitboards[isupper(state[i]) ? WHITE_ALL_PIECES : BLACK_ALL_PIECES] |= 1ULL << i;
        }
    }

    int bitIndex = _currentPlayer == WHITE ? WHITE_PAWNS : BLACK_PAWNS;
    int friendlyIndex = (_currentPlayer == WHITE) ? WHITE_ALL_PIECES : BLACK_ALL_PIECES;

    generateKnightMoves(moves, state);
    generateKingMoves(moves, state);
    generateBishopMoves(moves, _bitboards[WHITE_BISHOPS + bitIndex], _bitboards[OCCUPANCY].getData(), _bitboards[friendlyIndex].getData());
    generateRookMoves(moves, _bitboards[WHITE_ROOKS + bitIndex], _bitboards[OCCUPANCY].getData(), _bitboards[friendlyIndex].getData());
    generateQueenMoves(moves, _bitboards[WHITE_QUEENS + bitIndex], _bitboards[OCCUPANCY].getData(), _bitboards[friendlyIndex].getData());
    

    for (int index = 0; index < 64; index++) {
    if (state[index] == (_currentPlayer == WHITE ? 'P' : 'p')) {
        int row = index / 8;
        int col = index % 8;
        generatePawnMoves(moves, state, row, col, _currentPlayer);
    }
}

    return moves;
}

void Chess::generateKnightMoves(std::vector<BitMove> &moves, std::string &state){
    std::pair<int, int> knightMoves[8] = {
        {1, 2}, {2, 1}, {2, -1}, {1, -2},
        {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}
    };

    char knightPiece = _currentPlayer == WHITE ? 'N' : 'n';
    int index = 0;

    for (char square : state) {
        if (square == knightPiece) {
            int rank = index / 8;
            int file = index % 8;

            for (auto [df, dr] : knightMoves) {
                int r = rank + dr, f = file + df;
                if (r >= 0 && r < 8 && f >= 0 && f < 8){
                    moves.emplace_back(index, r * 8 + f, Knight);
                }
            }
        }

        index++;
    }
}

void Chess::generateKingMoves(std::vector<BitMove> &moves, std::string &state){
    std::pair<int, int> kingMoves[8] = {
        {0, 1}, {1, 1}, {1, 0}, {1, -1},
        {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}
    };

    char kingPiece = _currentPlayer == WHITE ? 'K' : 'k';
    int index = 0;

    for (char square : state) {
        if (square == kingPiece) {
            int rank = index / 8;
            int file = index % 8;

            for (auto [df, dr] : kingMoves) {
                int r = rank + dr, f = file + df;
                if (r >= 0 && r < 8 && f >= 0 && f < 8){
                    moves.emplace_back(index, r * 8 + f, King);
                }
            }
        }

        index++;
    }
}

void Chess::generatePawnMoves(std::vector<BitMove> &moves, std::string &state, int row, int col, int colorAsInt){
    int direction = colorAsInt == 1 ? 1 : -1;
    int startRow = colorAsInt == 1 ? 1 : 6;
    
    // Stop if pawn is already on last rank
    int nextRow = row + direction;
    if (nextRow < 0 || nextRow >= 8) return;

    // One square forward
    if (pieceNotation(col, row + direction) == '0'){
        moves.emplace_back(row * 8 + col, (row + direction) * 8 + col, Pawn);

        // Two squares forward from starting position
        if (row == startRow && pieceNotation(col, row + 2 * direction) == '0'){
            moves.emplace_back(row * 8 + col, (row + 2 * direction) * 8 + col, Pawn);
        }
    }

    // Captures
    for (int i = -1; i <= 1; i += 2) {
        if (col + i >= 0 && col + i < 8) {
            unsigned char piece = pieceNotation(col + i, row + direction);
            int pieceColor = (piece >= 'a' && piece <= 'z') ? -1 : (piece >= 'A' && piece <= 'Z') ? 1 : 0;
            if (pieceColor == -colorAsInt) {
                moves.emplace_back(row * 8 + col, (row + direction) * 8 + (col + i), Pawn);
            }
        }
    }
}

void Chess::generateBishopMoves(std::vector<BitMove> &moves, Bitboard piecesBoard, uint64_t occupancy, uint64_t friendlies){
    piecesBoard.forEachBit([&](int fromSquare) {
        Bitboard moveBoard = Bitboard(getBishopAttacks(fromSquare, occupancy) & ~friendlies);
        moveBoard.forEachBit([&](int toSquare) {
            moves.emplace_back(fromSquare, toSquare, Bishop);
        });
    });
}

void Chess::generateRookMoves(std::vector<BitMove> &moves, Bitboard piecesBoard, uint64_t occupancy, uint64_t friendlies){
    piecesBoard.forEachBit([&](int fromSquare) {
        Bitboard moveBoard = Bitboard(getRookAttacks(fromSquare, occupancy) & ~friendlies);
        moveBoard.forEachBit([&](int toSquare) {
            moves.emplace_back(fromSquare, toSquare, Rook);
        });
    });
}

void Chess::generateQueenMoves(std::vector<BitMove> &moves, Bitboard piecesBoard, uint64_t occupancy, uint64_t friendlies){
    piecesBoard.forEachBit([&](int fromSquare) {
        Bitboard moveBoard = Bitboard(getQueenAttacks(fromSquare, occupancy) & ~friendlies);
        moveBoard.forEachBit([&](int toSquare) {
            moves.emplace_back(fromSquare, toSquare, Queen);
        });
    });
}