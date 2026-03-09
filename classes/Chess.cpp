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
    setAIPlayer(1);

    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");

    _currentPlayer = WHITE;
    _moves = generateAllMoves();

    if (gameHasAI()) {
        setAIPlayer(AI_PLAYER);
    }

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

    if (gameHasAI() && getCurrentPlayer()->isAIPlayer()) {
        updateAI();
    }
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

void Chess::generateKnightMoves(std::vector<BitMove> &moves, std::string &state)
{
    int pieceIndex = (_currentPlayer == WHITE) ? WHITE_KNIGHTS : BLACK_KNIGHTS;
    int friendlyIndex = (_currentPlayer == WHITE) ? WHITE_ALL_PIECES : BLACK_ALL_PIECES;

    Bitboard piecesBoard = _bitboards[pieceIndex];

    piecesBoard.forEachBit([&](int fromSquare) {

        uint64_t attacks = KnightAttacks[fromSquare] & ~_bitboards[friendlyIndex].getData();
        Bitboard moveBoard(attacks);

        moveBoard.forEachBit([&](int toSquare) {
            moves.emplace_back(fromSquare, toSquare, Knight);
        });

    });
}

void Chess::generateKingMoves(std::vector<BitMove> &moves, std::string &state)
{
    int pieceIndex = (_currentPlayer == WHITE) ? WHITE_KING : BLACK_KING;
    int friendlyIndex = (_currentPlayer == WHITE) ? WHITE_ALL_PIECES : BLACK_ALL_PIECES;

    Bitboard piecesBoard = _bitboards[pieceIndex];

    piecesBoard.forEachBit([&](int fromSquare) {

        uint64_t attacks = KingAttacks[fromSquare] & ~_bitboards[friendlyIndex].getData();
        Bitboard moveBoard(attacks);

        moveBoard.forEachBit([&](int toSquare) {
            moves.emplace_back(fromSquare, toSquare, King);
        });

    });
}

void Chess::generatePawnMoves(std::vector<BitMove> &moves, std::string &state, int row, int col, int colorAsInt){
    int direction = colorAsInt == 1 ? 1 : -1;
    int startRow = colorAsInt == 1 ? 1 : 6;
    
    // Stop if pawn is already on last rank
    int nextRow = row + direction;
    if (nextRow < 0 || nextRow >= 8) return;

    // One square forward
    int forward = (row + direction) * 8 + col;

    if (state[forward] == '0') {
        moves.emplace_back(row * 8 + col, (row + direction) * 8 + col, Pawn);

        // Two squares forward from starting position
        int forward2 = (row + 2 * direction) * 8 + col;

        if (state[forward2] == '0' && row == startRow) {
            moves.emplace_back(row * 8 + col, (row + 2 * direction) * 8 + col, Pawn);
        }
    }

    // Captures
    for (int i = -1; i <= 1; i += 2) {
        if (col + i >= 0 && col + i < 8) {
            int target = (row + direction) * 8 + (col + i);
            char piece = state[target];
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




// CHESS AI --------------------------------------------------

void Chess::updateAI(){
    char baseState[65];
    const int myInfinity = 9999999;

    int bestMoveScore = -myInfinity;
    BitMove bestMove;

    std::string copyState = stateString();

    int playerColor = (getCurrentPlayer()->playerNumber() == 0 ? 1 : -1);

    for (auto move : _moves) {

        strcpy(baseState, copyState.c_str());

        int srcSquare = move.from;
        int dstSquare = move.to;

        baseState[dstSquare] = baseState[srcSquare];
        baseState[srcSquare] = '0';

        std::string newState(baseState);

        int score = -negamax(
            3,
            -myInfinity,
            myInfinity,
            newState,
            -playerColor
        );

        if (score > bestMoveScore) {
            bestMoveScore = score;
            bestMove = move;
        }
    }

    if (bestMoveScore != -myInfinity) {

        int srcSquare = bestMove.from;
        int dstSquare = bestMove.to;

        BitHolder& src = getHolderAt(srcSquare % 8, srcSquare / 8);
        BitHolder& dst = getHolderAt(dstSquare % 8, dstSquare / 8);

        Bit* bit = src.bit();
        if (!bit) return;

        dst.setBit(bit);
        src.setBit(nullptr);

        bit->setPosition(dst.getPosition());

        bitMovedFromTo(*bit, src, dst);
    }
}

int Chess::negamax(int depth, int alpha, int beta, std::string &state, int playerColor) {
    if (depth == 0){
        return evaluateBoard(state.c_str()) * playerColor;
    }

    int bestVal = -999999;

    int player = (playerColor == 1 ? WHITE : BLACK);

    auto moves = generateAllMovesFromState(state, player);

    for (auto move : moves) {

        int srcSquare = move.from;
        int dstSquare = move.to;

        char captured = state[dstSquare];

        state[dstSquare] = state[srcSquare];
        state[srcSquare] = '0';

        int value = -negamax(depth - 1, -beta, -alpha, state, -playerColor);

        state[srcSquare] = state[dstSquare];
        state[dstSquare] = captured;

        bestVal = std::max(bestVal, value);
        alpha = std::max(alpha, value);

        if (alpha >= beta)
            break;
    }

    return bestVal;
}

std::vector<BitMove> Chess::generateAllMovesFromState(std::string &state, int player) {
    std::vector<BitMove> moves;
    moves.reserve(32);

    for (int i = 0; i < e_numBitboards; i++)
        _bitboards[i] = 0;

    _bitboards[OCCUPANCY] = 0;
    _bitboards[WHITE_ALL_PIECES] = 0;
    _bitboards[BLACK_ALL_PIECES] = 0;

    for (int i = 0; i < 64; i++){

        int bitIndex = _bitboardLookup[state[i]];
        _bitboards[bitIndex] |= 1ULL << i;

        if (state[i] != '0'){
            _bitboards[OCCUPANCY] |= 1ULL << i;
            _bitboards[isupper(state[i]) ? WHITE_ALL_PIECES : BLACK_ALL_PIECES] |= 1ULL << i;
        }
    }

    int pawnIndex = player == WHITE ? WHITE_PAWNS : BLACK_PAWNS;
    int friendlyIndex = player == WHITE ? WHITE_ALL_PIECES : BLACK_ALL_PIECES;

    generateKnightMoves(moves, state);
    generateKingMoves(moves, state);

    generateBishopMoves(
        moves,
        _bitboards[WHITE_BISHOPS + pawnIndex],
        _bitboards[OCCUPANCY].getData(),
        _bitboards[friendlyIndex].getData()
    );

    generateRookMoves(
        moves,
        _bitboards[WHITE_ROOKS + pawnIndex],
        _bitboards[OCCUPANCY].getData(),
        _bitboards[friendlyIndex].getData()
    );

    generateQueenMoves(
        moves,
        _bitboards[WHITE_QUEENS + pawnIndex],
        _bitboards[OCCUPANCY].getData(),
        _bitboards[friendlyIndex].getData()
    );

    for (int index = 0; index < 64; index++) {

        if (state[index] == (player == WHITE ? 'P' : 'p')) {

            int row = index / 8;
            int col = index % 8;

            generatePawnMoves(moves, state, row, col, player);
        }
    }

    return moves;
}

/* piece/sq tables */

// piece square tables for every piece (from chess programming wiki)
const int pawnTable[64] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
    5, 5, 10, 25, 25, 10, 5, 5,
    0, 0, 0, 20, 20, 0, 0, 0,
    5, -5, -10, 0, 0, -10, -5, 5,
    5, 10, 10, -20, -20, 10, 10, 5,
    0, 0, 0, 0, 0, 0, 0, 0};
const int knightTable[64] = {
    -50, -40, -30, -30, -30, -30, -40, -50,
    -40, -20, 0, 0, 0, 0, -20, -40,
    -30, 0, 10, 15, 15, 10, 0, -30,
    -30, 5, 15, 20, 20, 15, 5, -30,
    -30, 0, 15, 20, 20, 15, 0, -30,
    -30, 5, 10, 15, 15, 10, 5, -30,
    -40, -20, 0, 5, 5, 0, -20, -40,
    -50, -40, -30, -30, -30, -30, -40, -50};
const int rookTable[64] = {
    0, 0, 0, 5, 5, 0, 0, 0,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    -5, 0, 0, 0, 0, 0, 0, -5,
    5, 10, 10, 10, 10, 10, 10, 5,
    0, 0, 0, 0, 0, 0, 0, 0};
const int queenTable[64] = {
    -20, -10, -10, -5, -5, -10, -10, -20,
    -10, 0, 0, 0, 0, 0, 0, -10,
    -10, 0, 5, 5, 5, 5, 0, -10,
    -5, 0, 5, 5, 5, 5, 0, -5,
    0, 0, 5, 5, 5, 5, 0, -5,
    -10, 5, 5, 5, 5, 5, 0, -10,
    -10, 0, 5, 0, 0, 0, 0, -10,
    -20, -10, -10, -5, -5, -10, -10, -20};
const int kingTable[64] = {
    20, 30, 10, 0, 0, 10, 30, 20,
    20, 20, 0, 0, 0, 0, 20, 20,
    -10, -20, -20, -20, -20, -20, -20, -10,
    -20, -30, -30, -40, -40, -30, -30, -20,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30};
const int bishopTable[64] = {
    -20, -10, -10, -10, -10, -10, -10, -20,
    -10, 0, 0, 0, 0, 0, 0, -10,
    -10, 0, 5, 10, 10, 5, 0, -10,
    -10, 5, 5, 10, 10, 5, 5, -10,
    -10, 0, 10, 10, 10, 10, 0, -10,
    -10, 10, 10, 10, 10, 10, 10, -10,
    -10, 5, 0, 0, 0, 0, 5, -10,
    -20, -10, -10, -10, -10, -10, -10, -20};


//
//
// This is just a sample evaluator that you can use, it may not be copy/paste compatible with your code so 
// make sure you adjust it properly.
//


static std::map<char, int> evaluateScores = {
        {'P', 100}, {'p', -100},
        {'N', 200}, {'n', -200},
        {'B', 230}, {'b', -230},
        {'R', 400}, {'r', -400},
        {'Q', 900}, {'q', -900},
        {'K', 2000}, {'k', -2000},
        {'0', 0}
};

inline int FLIP(int index) {
    return index ^ 56;
}

inline int Chess::evaluateBoard(const char* state) {
    int score = 0;
    for (int i=0; i<64; i++) {
        score += evaluateScores[state[i]];
    }
    for (int i=0; i<64; i++) {
        char piece = state[i];
        int j = FLIP(i);
        switch (piece) {
            case 'N': // Knight
                score += knightTable[j];
                break;
            case 'n':
                score -= knightTable[FLIP(j)];
                break;
            case 'P': // Knight
                score += pawnTable[j];
                break;
            case 'p':
                score -= pawnTable[FLIP(j)];
                break;
            case 'K': // Knight
                score += kingTable[j];
                break;
            case 'k':
                score -= kingTable[FLIP(j)];
                break;
            case 'R': // Knight
                score += rookTable[j];
                break;
            case 'r':
                score -= rookTable[FLIP(j)];
                break;
            case 'Q': // Knight
                score += queenTable[j];
                break;
            case 'q':
                score -= queenTable[FLIP(j)];
                break;
            case 'B':
                score += bishopTable[j];
                break;
            case 'b':
                score -= bishopTable[FLIP(j)];
                break;
        }
    }
    return score;
}