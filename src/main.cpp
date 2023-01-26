#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <fstream>
#include <numeric>
#include <random>

#include "renderer.hpp"

int main(int argc, char **argv) {
    srand(time(nullptr));

    std::string save, load;
    xt::Team    player = xt::Team::MAX;
    for (int i = 0; i < argc; i++) {
        std::string_view arg{argv[i]};
        if (arg == "-s" && i + 1 < argc)
            save = argv[++i];
        if (arg == "-l" && i + 1 < argc)
            load = argv[++i];
        if (arg == "-w")
            player = xt::Team::WHITE;
        if (arg == "-b")
            player = xt::Team::BLACK;
        if (arg == "-r")
            player = (xt::Team)(rand() % xt::Team::MAX);
    }

    sf::RenderWindow window(
        sf::VideoMode(xt::BoardRenderer::BOARD_SIZE, xt::BoardRenderer::BOARD_SIZE),
        "",
        sf::Style::Close);
    window.setVerticalSyncEnabled(true);

    sf::Clock clock;
    sf::Time  last, update;

    xt::Board         board;
    xt::BoardRenderer renderer(board);
    renderer.SetPosition(sf::Vector2f{0.f, 0.f});
    while (window.isOpen()) {
        const auto now = clock.getElapsedTime();
        const auto fps = 1.f / (now - last).asSeconds();
        last           = now;

        sf::Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {
            case sf::Event::Closed:
                window.close();
                break;
            case sf::Event::KeyReleased:
                if (!event.key.control)
                    break;

                switch (event.key.code) {
                case sf::Keyboard::S:
                {
                    std::ofstream file(save, std::ios::binary);
                    if (file.good()) {
                        const auto data = board.Save();
                        file.write(reinterpret_cast<const char *>(data.data()), data.size());

                        fmt::print("Saved to '{}'!\n", save);
                    }
                } break;
                case sf::Keyboard::L:
                {
                    std::ifstream file(load, std::ios::binary | std::ios::ate);
                    if (file.good()) {
                        const auto size = file.tellg();
                        file.seekg(std::ios::beg);

                        std::vector<std::uint8_t> data(size, '\0');
                        file.read(reinterpret_cast<char *>(data.data()), data.size());

                        if (board.Load(data)) {
                            renderer.UpdateTitle();
                            fmt::print("Loaded from '{}'!\n", load);
                            break;
                        }
                    }

                    fmt::print("Load from '{}' failed!\n", load);
                } break;
                default:
                    break;
                }

                break;
            default:
                break;
            }

            renderer.ProcessEvent(window, event);
        }

        window.clear(sf::Color::Black);
        renderer.Render(window);
        window.display();

        static std::string status;
        if ((clock.getElapsedTime() - update).asSeconds() > 1.f) {
            update = now;
            window.setTitle(fmt::format("{} ({:.0f} FPS)", status, fps));
        }

        if (status != renderer.GetTitle()) {
            update = now;
            status = renderer.GetTitle();
            window.setTitle(fmt::format("{} ({:.0f} FPS)", status, fps));
        }

        // 'ai'
        if (const auto piece = board.GetPromoting())
            if (piece->team == player)
                board.Promote(xt::Piece::QUEEN);

        if (board.GetTurn() == player) {
            const auto moves = board.GetValidMoves(board.GetTurn());
            if (!moves.empty()) {
                const auto &move = moves[rand() % moves.size()];
                if (board.TryMove(move.first, move.second))
                    renderer.UpdateTitle();
            }
        }
    }

    return 0;
}