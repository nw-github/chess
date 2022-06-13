#pragma once

#include <vector>
#include <string>
#include <optional>

namespace zc
{
    struct Position
    {
        int x;
        int y;

    public:
        Position(int x, int y) : x(x), y(y) { }
        Position() : Position(0, 0) { }

        Position &operator+=(const Position &other)
        {
            x += other.x;
            y += other.y;
            return *this;
        }

        bool operator==(const Position &other) const
        {
            return x == other.x && y == other.y;
        }

        bool operator!=(const Position &other) const
        {
            return !(*this == other);
        }
    };

    inline const Position INVALID_POS{-1, -1};

    struct Piece
    {
        enum Type
        {
            QUEEN,
            KING,
            ROOK,
            KNIGHT,
            BISHOP,
            PAWN,
            MAX
        } type{MAX};
        int team{INVALID_TEAM};
        bool moved{false};
        Position enPassant{INVALID_POS};

    public:
        bool OpposingTeam(int other) const;
        bool IsEmpty() const;
        void Move(Piece &other);
        void Clear();

    private:
        static constexpr int INVALID_TEAM{-1};

    };

    class Board
    {
    public:
        static constexpr const int SIZE = 8;
        static constexpr const int PLAYERW = 1;
        static constexpr const int PLAYERB = 0;

        enum Status
        {
            ACTIVE,
            CHECKMATE,
            STALEMATE
        };

    public:
        Board();

    public:
        int GetTurn() const;
        Position GetKing(int team) const;
        const Piece *GetPromoting() const;

        const Piece &operator[](const Position &pos) const;

        Status GetStatus() const;

        bool IsValid(int x, int y) const;
        bool IsValid(const Position &pos) const;

        bool IsValidMove(const Position &src, const Position &dest) const;

        bool TryMove(const Position &src, const Position &dest);
        void Promote(Piece::Type type);

        std::vector<std::pair<Position, Position>> GetValidMoves(int team) const;

    private:
        // Chess notation (ie 'E4')
        Piece &At(char col, int row);

        Piece &operator()(int x, int y);
        Piece &operator()(const Position &pos);

        const Piece &operator()(int x, int y) const;
        const Piece &operator()(const Position &pos) const;

        // For pieces that move straight or diagonally, ensure there are no pieces in the way
        bool TracePath(Position src, const Position &dest, bool castling = false) const;
        bool IsInCheck(int team, const Position &dest) const;
        bool IsKingInCheck(int team) const;

        int GetValidMoveCount(int team) const;

        void Move(const Position &src, const Position &king);
        void NextTurn();

        // returns position of the rook to castle with if valid
        std::optional<Position> IsCastlingMove(const Position &src, const Position &dest) const;

    private:
        Piece    mBoard[SIZE * SIZE];
        Position mPromoting{INVALID_POS};
        int      mTurn{PLAYERW};
    };
}