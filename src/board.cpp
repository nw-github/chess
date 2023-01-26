#include "board.hpp"

#include <algorithm>
#include <fmt/format.h>

namespace xt {
    bool Piece::OpposingTeam(Team other) const {
        return team != other && other != Team::MAX && team != Team::MAX;
    }

    bool Piece::IsEmpty() const {
        return team == Team::MAX || type == Type::MAX;
    }

    void Piece::Clear() {
        team = Team::MAX;
        type = Type::MAX;
    }

    void Piece::Move(Piece &other) {
        other       = *this;
        other.moved = true;

        Clear();
    }
} // namespace xt

namespace xt {
    Board::Board() {
        Initialize({
            {'A', Piece::Type::ROOK},
            {'B', Piece::Type::KNIGHT},
            {'C', Piece::Type::BISHOP},
            {'D', Piece::Type::QUEEN},
            {'E', Piece::Type::KING},
            {'F', Piece::Type::BISHOP},
            {'G', Piece::Type::KNIGHT},
            {'H', Piece::Type::ROOK},
        });
    }

    void Board::Initialize(const std::vector<std::pair<char, Piece::Type>> &rear) {
        const auto InitSide = [&](Int pawns, Int back, Team team) {
            for (const auto &pair : rear)
                At(pair.first, back) = Piece{pair.second, team};

            for (char c = 'A'; c < 'A' + SIZE; c++)
                At(c, pawns) = Piece{Piece::Type::PAWN, team};
        };

        InitSide(2, 1, Team::WHITE);
        InitSide(SIZE - 1, SIZE, Team::BLACK);
    }

    // Utils
    Vector Board::GetKing(Team team) const {
        const auto it = std::find_if(std::begin(mBoard), std::end(mBoard), [team](const auto &a) {
            return a.type == Piece::KING && a.team == team;
        });

        if (it == std::end(mBoard))
            return INVALID_POS;

        const auto index = std::distance(std::begin(mBoard), it);
        return {static_cast<Int>(index % SIZE), static_cast<Int>(index / SIZE)};
    }

    Team Board::GetTurn() const {
        return mTurn;
    }

    Piece &Board::At(char col, Int row) {
        return (*this)(col - 'A', SIZE - row);
    }

    std::optional<Piece> Board::GetPromoting() const {
        if (IsValid(mPromoting))
            return (*this)(mPromoting);
        return std::nullopt;
    }

    Piece &Board::operator()(Int x, Int y) {
        if (!IsValid(x, y))
            throw std::runtime_error(fmt::format("Invalid board coordinates ({}, {})", x, y));

        return mBoard[y * SIZE + x];
    }

    Piece &Board::operator()(const Vector &pos) {
        return (*this)(pos.x, pos.y);
    }

    const Piece &Board::operator()(Int x, Int y) const {
        if (!IsValid(x, y))
            throw std::runtime_error(fmt::format("Invalid board coordinates ({}, {})", x, y));

        return mBoard[y * SIZE + x];
    }

    const Piece &Board::operator()(const Vector &pos) const {
        return (*this)(pos.x, pos.y);
    }

    const Piece &Board::operator[](const Vector &pos) const {
        return (*this)(pos.x, pos.y);
    }

    Board::Status Board::GetStatus() const {
        if (!GetValidMoveCount(mTurn))
            return IsKingInCheck(mTurn) ? Status::CHECKMATE : Status::STALEMATE;
        return Status::ACTIVE;
    }

    bool Board::IsValid(Int x, Int y) const {
        return x < SIZE && y < SIZE && x >= 0 && y >= 0;
    }

    bool Board::IsValid(const Vector &pos) const {
        return IsValid(pos.x, pos.y);
    }

    // Data

    std::vector<std::uint8_t> Board::Save() const {
        std::vector<std::uint8_t> data(sizeof(*this), '\0');
        std::memcpy(data.data(), this, data.size());
        return data;
    }

    bool Board::Load(const std::vector<std::uint8_t> &data) {
        if (data.size() != sizeof(*this))
            return false;

        std::memcpy(this, data.data(), data.size());
        return true;
    }

    // Logic

    void Board::NextTurn() {
        mTurn = mTurn == Team::WHITE ? Team::BLACK : Team::WHITE;
    }

    void Board::Promote(Piece::Type type) {
        if (IsValid(mPromoting)) {
            (*this)(mPromoting).type = type;
            mPromoting               = INVALID_POS;

            NextTurn();
        }
    }

    void Board::Move(const Vector &src, const Vector &dest) {
        auto &piece  = (*this)(src);
        auto &target = (*this)(dest);
        if (piece.type == Piece::PAWN && IsValid(piece.enPassant) && src.x - dest.x != 0 &&
            target.IsEmpty())
            (*this)(piece.enPassant).Clear();

        for (auto &piece : mBoard)
            piece.enPassant = INVALID_POS;

        switch (piece.type) {
        case Piece::PAWN:
            if (abs(src.y - dest.y) == 2) {
                for (auto x = dest.x - 1; x <= dest.x + 1; x++) {
                    if (!IsValid(x, dest.y))
                        continue;

                    auto &enemy = (*this)(x, dest.y);
                    if (enemy.type == Piece::PAWN && piece.OpposingTeam(enemy.team))
                        enemy.enPassant = dest;
                }
            }

            if ((piece.team == Team::WHITE && dest.y == 0) ||
                (piece.team == Team::BLACK && dest.y == SIZE - 1))
                mPromoting = dest;

            break;
        case Piece::KING:
            if (auto rook = IsCastlingMove(src, dest))
                (*this)(*rook).Move((*this)((dest.x - src.x) > 0 ? dest.x - 1 : dest.x + 1, src.y));

            break;
        default:
            break;
        }

        piece.Move(target);
    }

    bool Board::TryMove(const Vector &src, const Vector &dest) {
        auto &piece = (*this)(src);
        if (piece.team != mTurn)
            return false;

        if (IsValidMove(src, dest)) {
            Move(src, dest);

            if (!IsValid(mPromoting))
                NextTurn();

            return true;
        }

        return false;
    }

    // Helpers

    bool Board::IsValidMove(const Vector &src, const Vector &dest) const {
        if (src == dest)
            return false;

        const auto &piece = (*this)(src);
        if (piece.team == (*this)(dest).team)
            return false;

        const Vector dist(abs(dest.x - src.x), abs(dest.y - src.y));
        switch (piece.type) {
        case Piece::KING:
            if (dist.y > 1)
                return false;

            if (dist.x > 1) {
                const auto rook = IsCastlingMove(src, dest);
                if (!rook)
                    return false;

                const auto &rp = (*this)(*rook);
                if (rp.IsEmpty() || rp.moved)
                    return false;

                const Vector dir{static_cast<Int>((dest.x - src.x) / dist.x), 0};

                Vector now = src;
                while ((now += dir) != *rook) {
                    if (!(*this)(now).IsEmpty())
                        return false;
                    if ((now.x - rook->x) < (now.x - dest.x) && IsInCheck(piece.team, src))
                        return false;
                }
            }

            break;
        case Piece::QUEEN:
            if (dist.x != dist.y && dist.x != 0 && dist.y != 0)
                return false;

            if (!TracePath(src, dest))
                return false;

            break;
        case Piece::ROOK:
            if (dist.x != 0 && dist.y != 0)
                return false;

            if (!TracePath(src, dest))
                return false;

            break;
        case Piece::KNIGHT:
            if (!(dist.x == 2 && dist.y == 1) && !(dist.x == 1 && dist.y == 2))
                return false;

            break;
        case Piece::BISHOP:
            if (dist.x != dist.y)
                return false;

            if (!TracePath(src, dest))
                return false;

            break;
        case Piece::PAWN:
            if (dist.x > 1 || dist.y > 1 || dist.y == 0)
                if (dist.y != 2 || dist.x != 0 || piece.moved || !TracePath(src, dest))
                    return false;

            if (dist.x > 0) {
                bool capturing = piece.OpposingTeam((*this)(dest).team);
                if (!capturing && !IsValid(piece.enPassant))
                    return false;

                if (!capturing && IsValid(piece.enPassant) &&
                    abs(dest.x - piece.enPassant.x) > abs(src.x - piece.enPassant.x))
                    return false;

            } else if (!(*this)(dest).IsEmpty()) {
                return false;
            }

            if ((dest.y - src.y > 0 && piece.team == Team::WHITE) ||
                (dest.y - src.y < 0 && piece.team == Team::BLACK))
                return false;

            break;
        default:
            return false;
        }

        Board copy{*this};
        copy.Move(src, dest);
        return !copy.IsKingInCheck(piece.team);
    }

    bool Board::TracePath(Vector src, const Vector &dest) const {
        Vector dir(dest.x - src.x, dest.y - src.y);
        if (dir.x != 0)
            dir.x /= abs(dir.x);
        if (dir.y != 0)
            dir.y /= abs(dir.y);

        while ((src += dir) != dest)
            if (!(*this)(src).IsEmpty())
                return false;

        return true;
    }

    bool Board::IsInCheck(Team team, const Vector &king) const {
        for (Int y = 0; y < SIZE; y++)
            for (Int x = 0; x < SIZE; x++)
                if ((*this)(x, y).OpposingTeam(team) && IsValidMove({x, y}, king))
                    return true;

        return false;
    }

    bool Board::IsKingInCheck(Team team) const {
        const auto king = GetKing(team);

        // King captured
        if (!IsValid(king))
            return true;

        return IsInCheck(team, king);
    }

    std::size_t Board::GetValidMoveCount(Team team) const {
        std::size_t moves = 0;
        for (Int y = 0; y < SIZE; y++) {
            for (Int x = 0; x < SIZE; x++) {
                const auto &piece = (*this)(x, y);
                if (piece.team != team)
                    continue;

                // TODO: this is lazy
                for (Int ry = 0; ry < SIZE; ry++)
                    for (Int rx = 0; rx < SIZE; rx++)
                        if (IsValidMove({x, y}, {rx, ry}))
                            moves++;
            }
        }

        return moves;
    }

    std::vector<std::pair<Vector, Vector>> Board::GetValidMoves(Team team) const {
        std::vector<std::pair<Vector, Vector>> moves;

        for (Int y = 0; y < SIZE; y++) {
            for (Int x = 0; x < SIZE; x++) {
                const auto &piece = (*this)(x, y);
                if (piece.team != team)
                    continue;

                // TODO: this is lazy
                for (Int ry = 0; ry < SIZE; ry++)
                    for (Int rx = 0; rx < SIZE; rx++)
                        if (IsValidMove({x, y}, {rx, ry}))
                            moves.push_back({{x, y}, {rx, ry}});
            }
        }

        return moves;
    }

    std::optional<Vector> Board::IsCastlingMove(const Vector &src, const Vector &dest) const {
        const auto &king = (*this)(src);
        if (abs(dest.x - src.x) != 2 || (dest.y - src.y) != 0 || king.moved ||
            IsInCheck(king.team, src))
            return std::nullopt;

        return Vector{static_cast<Int>((dest.x - src.x) > 0 ? SIZE - 1 : 0), src.y};
    }
} // namespace xt