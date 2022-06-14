#pragma once

#include <vector>
#include <string>
#include <optional>

namespace zc
{
    using Int = std::int8_t;

    enum Team : std::int8_t
    {
        BLACK,
        WHITE,
        MAX,
    };

    struct Position
    {
        Int x;
        Int y;

    public:
        Position(Int x, Int y) : x(x), y(y) { }
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
        enum Type : std::int8_t
        {
            QUEEN,
            KING,
            ROOK,
            KNIGHT,
            BISHOP,
            PAWN,
            MAX
        };
        
        Type     type{Type::MAX};
        Team     team{Team::MAX};
        bool     moved{false};
        Position enPassant{INVALID_POS};

    public:
        bool OpposingTeam(Team other) const;
        bool IsEmpty() const;
        void Move(Piece &other);
        void Clear();

    private:
        static constexpr Int INVALID_TEAM{-1};

    };

    class Board
    {
    public:
        static constexpr const Int SIZE = 8;

        enum Status
        {
            ACTIVE,
            CHECKMATE,
            STALEMATE
        };

    public:
        Board();

    public:
        Team GetTurn() const;
        Position GetKing(Team team) const;
        const Piece *GetPromoting() const;

        const Piece &operator[](const Position &pos) const;

        Status GetStatus() const;

        bool IsValid(Int x, Int y) const;
        bool IsValid(const Position &pos) const;

        bool IsValidMove(const Position &src, const Position &dest) const;

        bool TryMove(const Position &src, const Position &dest);
        void Promote(Piece::Type type);

        // Chess notation (ie 'E4')
        Piece &At(char col, Int row);

        std::vector<std::pair<Position, Position>> GetValidMoves(Team team) const;
    
    public:
        std::vector<std::uint8_t> Save() const;
        bool Load(const std::vector<std::uint8_t> &data);

    private:
        Piece &operator()(Int x, Int y);
        Piece &operator()(const Position &pos);

        const Piece &operator()(Int x, Int y) const;
        const Piece &operator()(const Position &pos) const;

        // For pieces that move straight or diagonally, ensure there are no pieces in the way
        bool TracePath(Position src, const Position &dest, bool castling = false) const;
        bool IsInCheck(Team team, const Position &dest) const;
        bool IsKingInCheck(Team team) const;

        std::size_t GetValidMoveCount(Team team) const;

        void Move(const Position &src, const Position &king);
        void NextTurn();

        // returns position of the rook to castle with if valid
        std::optional<Position> IsCastlingMove(const Position &src, const Position &dest) const;

    private:
        Piece    mBoard[SIZE * SIZE];
        Position mPromoting{INVALID_POS};
        Team     mTurn{Team::WHITE};

    };
}