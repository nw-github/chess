#include "board.hpp"

#include <fmt/format.h>
#include <algorithm>

namespace zc
{
    bool Piece::OpposingTeam(int other) const
    {
        return team != other && other != INVALID_TEAM && team != INVALID_TEAM;
    }

    bool Piece::IsEmpty() const
    {
        return team == INVALID_TEAM || type == Piece::MAX;
    }

    void Piece::Clear()
    {
        team = INVALID_TEAM;
        type = Piece::MAX;
    }

    void Piece::Move(Piece &other)
    {
        other = *this;
        other.moved = true;

        Clear();
    }
}

namespace zc
{
    Board::Board()
    {
        auto InitSide = [&](int pawns, int back, int team)
        {
            At('A', back) = At('H', back) = Piece{Piece::Type::ROOK, team};
            At('B', back) = At('G', back) = Piece{Piece::Type::KNIGHT, team};
            At('C', back) = At('F', back) = Piece{Piece::Type::BISHOP, team};
            At('D', back) = Piece{Piece::Type::QUEEN, team};
            At('E', back) = Piece{Piece::Type::KING, team};

            for (char c = 'A'; c < 'A' + SIZE; c++)
                At(c, pawns) = Piece{Piece::Type::PAWN, team};
        };

        InitSide(SIZE - 1, SIZE, PLAYERB);
        InitSide(2, 1, PLAYERW);
    }

    // Utils

    Position Board::GetKing(int team) const
    {
        const auto it = std::find_if(std::begin(mBoard), std::end(mBoard),
            [team](const auto &a)
            {
                return a.type == Piece::KING && a.team == team;
            });

        if (it == std::end(mBoard))
            return INVALID_POS;

        const int index = std::distance(std::begin(mBoard), it);
        return {index % SIZE, index / SIZE};
    }

    int Board::GetTurn() const
    {
        return mTurn;
    }

    Piece &Board::At(char col, int row)
    {
        return (*this)(col - 'A', SIZE - row);
    }

    const Piece *Board::GetPromoting() const
    {
        if (IsValid(mPromoting))
            return &(*this)(mPromoting);
        return nullptr;
    }

    Piece &Board::operator()(int x, int y)
    {
        if (!IsValid(x, y))
            throw std::runtime_error(fmt::format("Invalid board coordinates ({}, {})", x, y));

        return mBoard[y * SIZE + x];
    }

    Piece &Board::operator()(const Position &pos)
    {
        return (*this)(pos.x, pos.y);
    }

    const Piece &Board::operator()(int x, int y) const
    {
        if (!IsValid(x, y))
            throw std::runtime_error(fmt::format("Invalid board coordinates ({}, {})", x, y));

        return mBoard[y * SIZE + x];
    }

    const Piece &Board::operator()(const Position &pos) const
    {
        return (*this)(pos.x, pos.y);
    }

    const Piece &Board::operator[](const Position &pos) const
    {
        return (*this)(pos.x, pos.y);
    }

    Board::Status Board::GetStatus() const
    {
        if (!GetValidMoveCount(mTurn))
            return IsKingInCheck(mTurn) ? Status::CHECKMATE : Status::STALEMATE;
        return Status::ACTIVE;
    }

    bool Board::IsValid(int x, int y) const
    {
        return x < SIZE &&y < SIZE &&x >= 0 && y >= 0;
    }

    bool Board::IsValid(const Position &pos) const
    {
        return IsValid(pos.x, pos.y);
    }

    // Logic

    void Board::NextTurn()
    {
        mTurn = mTurn == PLAYERW ? PLAYERB : PLAYERW;
    }

    void Board::Promote(Piece::Type type)
    {
        if (IsValid(mPromoting))
        {
            (*this)(mPromoting).type = type;
            mPromoting = INVALID_POS;

            NextTurn();
        }
    }

    void Board::Move(const Position &src, const Position &dest)
    {
        auto &piece  = (*this)(src);
        auto &target = (*this)(dest);
        if (piece.type == Piece::PAWN && IsValid(piece.enPassant) && src.x - dest.x != 0 && target.IsEmpty())
            (*this)(piece.enPassant).Clear();

        for (auto &piece : mBoard)
            piece.enPassant = INVALID_POS;

        switch (piece.type)
        {
        case Piece::PAWN:
            if (abs(src.y - dest.y) == 2)
            {
                for (int x = dest.x - 1; x <= dest.x + 1; x++)
                {
                    if (!IsValid(x, dest.y))
                        continue;

                    auto &enemy = (*this)(x, dest.y);
                    if (enemy.type == Piece::PAWN && piece.OpposingTeam(enemy.team))
                        enemy.enPassant = dest;
                }
            }

            if ((piece.team == PLAYERW && dest.y == 0) || (piece.team == PLAYERB && dest.y == SIZE - 1))
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

    bool Board::TryMove(const Position &src, const Position &dest)
    {
        auto &piece = (*this)(src);
        if (piece.team != mTurn)
            return false;

        if (IsValidMove(src, dest))
        {
            Move(src, dest);
            
            if (!IsValid(mPromoting))
                NextTurn();

            return true;
        }

        return false;
    }

    // Helpers

    bool Board::IsValidMove(const Position &src, const Position &dest) const
    {
        if (src == dest)
            return false;

        const auto &piece = (*this)(src);
        if (piece.team == (*this)(dest).team)
            return false;

        const Position dist(abs(dest.x - src.x), abs(dest.y - src.y));
        switch (piece.type)
        {
        case Piece::KING:
            if (dist.y > 1)
                return false;

            if (dist.x > 1)
            {
                const auto rook = IsCastlingMove(src, dest);
                if (!rook)
                    return false;
                if ((*this)(*rook).moved || !TracePath(src, *rook, true))
                    return {};
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

            if (dist.x > 0)
            {
                bool capturing = piece.OpposingTeam((*this)(dest).team);
                if (!capturing && !IsValid(piece.enPassant))
                    return false;

                if (!capturing && IsValid(piece.enPassant) && abs(dest.x - piece.enPassant.x) > abs(src.x - piece.enPassant.x))
                    return false;

            } else if (!(*this)(dest).IsEmpty())
            {
                return false;
            }

            if ((dest.y - src.y > 0 && piece.team == PLAYERW) || (dest.y - src.y < 0 && piece.team == PLAYERB))
                return false;

            break;
        default:
            return false;
        }

        Board copy{*this};
        copy.Move(src, dest);
        return !copy.IsKingInCheck(piece.team);
    }

    bool Board::TracePath(Position src, const Position &dest, bool castling) const
    {
        Position dir(dest.x - src.x, dest.y - src.y);
        if (dir.x != 0)
            dir.x /= abs(dir.x);
        if (dir.y != 0)
            dir.y /= abs(dir.y);

        const auto team = (*this)(src).team;
        while ((src += dir) != dest)
            if (!(*this)(src).IsEmpty() || (castling && IsInCheck(team, src)))
                return false;

        return true;
    }

    bool Board::IsInCheck(int team, const Position &king) const
    {
        for (int y = 0; y < SIZE; y++)
            for (int x = 0; x < SIZE; x++)
                if ((*this)(x, y).OpposingTeam(team) && IsValidMove({x, y}, king))
                    return true;

        return false;
    }

    bool Board::IsKingInCheck(int team) const
    {
        const auto king = GetKing(team);

        // King captured
        if (!IsValid(king))
            return true;

        return IsInCheck(team, king);
    }

    int Board::GetValidMoveCount(int team) const
    {
        int moves = 0;
        for (int y = 0; y < SIZE; y++)
        {
            for (int x = 0; x < SIZE; x++)
            {
                const auto &piece = (*this)(x, y);
                if (piece.team != team)
                    continue;

                // TODO: this is lazy
                for (int ry = 0; ry < SIZE; ry++)
                    for (int rx = 0; rx < SIZE; rx++)
                        if (IsValidMove({x, y}, {rx, ry}))
                            moves++;
            }
        }

        return moves;
    }

    std::vector<std::pair<Position, Position>> Board::GetValidMoves(int team) const
    {
        std::vector<std::pair<Position, Position>> moves;

        for (int y = 0; y < SIZE; y++)
        {
            for (int x = 0; x < SIZE; x++)
            {
                const auto &piece = (*this)(x, y);
                if (piece.team != team)
                    continue;

                // TODO: this is lazy
                for (int ry = 0; ry < SIZE; ry++)
                    for (int rx = 0; rx < SIZE; rx++)
                        if (IsValidMove({x, y}, {rx, ry}))
                            moves.push_back({{x, y}, {rx, ry}});
            }
        }

        return moves;
    }

    std::optional<Position> Board::IsCastlingMove(const Position &src, const Position &dest) const
    {
        const auto &king = (*this)(src);
        if (abs(dest.x - src.x) != 2 || (dest.y - src.y) != 0 || king.moved || IsInCheck(king.team, src))
            return {};

        return Position{(dest.x - src.x) > 0 ? SIZE - 1 : 0, src.y};
    }
}