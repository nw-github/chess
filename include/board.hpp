#pragma once

#include <optional>
#include <string>
#include <vector>

namespace xt {
    using Int = std::int8_t;

    enum Team : std::int8_t {
        BLACK,
        WHITE,
        MAX,
    };

    struct Vector {
        Int x;
        Int y;

    public:
        Vector(Int x, Int y) : x(x), y(y) { }
        Vector() : Vector(0, 0) { }

        Vector &operator+=(const Vector &other) {
            x += other.x;
            y += other.y;
            return *this;
        }

        bool operator==(const Vector &other) const {
            return x == other.x && y == other.y;
        }

        bool operator!=(const Vector &other) const {
            return !(*this == other);
        }
    };

    inline const Vector INVALID_POS{-1, -1};

    struct Piece {
        enum Type : std::int8_t { QUEEN, KING, ROOK, KNIGHT, BISHOP, PAWN, MAX };

        Type   type{Type::MAX};
        Team   team{Team::MAX};
        bool   moved{false};
        Vector enPassant{INVALID_POS};

    public:
        bool OpposingTeam(Team other) const;
        bool IsEmpty() const;
        void Move(Piece &other);
        void Clear();

    private:
        static constexpr Int INVALID_TEAM{-1};
    };

    class Board {
    public:
        static constexpr const Int SIZE = 8;

        enum Status { ACTIVE, CHECKMATE, STALEMATE };

    public:
        Board();
        void Initialize(const std::vector<std::pair<char, Piece::Type>> &rear);

    public:
        Team         GetTurn() const;
        Vector       GetKing(Team team) const;
        const Piece *GetPromoting() const;

        const Piece &operator[](const Vector &pos) const;

        Status GetStatus() const;

        bool IsValid(Int x, Int y) const;
        bool IsValid(const Vector &pos) const;
        bool IsValidMove(const Vector &src, const Vector &dest) const;

        std::vector<std::pair<Vector, Vector>> GetValidMoves(Team team) const;

        bool TryMove(const Vector &src, const Vector &dest);
        void Promote(Piece::Type type);

    public:
        std::vector<std::uint8_t> Save() const;
        bool                      Load(const std::vector<std::uint8_t> &data);

    private:
        // Chess notation (ie 'E4')
        Piece &At(char col, Int row);

        Piece &operator()(Int x, Int y);
        Piece &operator()(const Vector &pos);

        const Piece &operator()(Int x, Int y) const;
        const Piece &operator()(const Vector &pos) const;

        // For a straight or diagonal move, ensure there are no pieces between src and dest
        bool TracePath(Vector src, const Vector &dest) const;
        bool IsInCheck(Team team, const Vector &dest) const;
        bool IsKingInCheck(Team team) const;

        std::size_t GetValidMoveCount(Team team) const;

        void Move(const Vector &src, const Vector &king);
        void NextTurn();

        // returns position of the rook to castle with if valid
        std::optional<Vector> IsCastlingMove(const Vector &src, const Vector &dest) const;

    private:
        Piece  mBoard[SIZE * SIZE];
        Vector mPromoting{INVALID_POS};
        Team   mTurn{Team::WHITE};
    };
} // namespace xt