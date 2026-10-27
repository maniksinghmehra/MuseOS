#include <iostream>
#include <limits>
#include <string>
#include <sqlite3.h>
#include "os_simulator.h"

int printRow(void*, int count, char** values, char** columns) {
    for (int i = 0; i < count; i++) {
        std::cout << columns[i] << ": "
                  << (values[i] ? values[i] : "NULL") << " | ";
    }
    std::cout << '\n';
    return 0;
}

const char* textColumn(sqlite3_stmt* stmt, int column) {
    const unsigned char* value = sqlite3_column_text(stmt, column);
    return value ? reinterpret_cast<const char*>(value) : "NULL";
}

bool readNumber(const char* prompt, int& value) {
    std::cout << prompt;
    if (std::cin >> value) {
        return true;
    }

    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "Please enter a number.\n";
    return false;
}

void readLine(const char* prompt, std::string& value) {
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << prompt;
    std::getline(std::cin, value);
}

void runQuery(sqlite3* db, const char* sql) {
    char* errorMessage = nullptr;
    int result = sqlite3_exec(db, sql, printRow, nullptr, &errorMessage);

    if (result != SQLITE_OK) {
        std::cerr << "Query failed: "
                  << (errorMessage ? errorMessage : "unknown error") << '\n';
        sqlite3_free(errorMessage);
    }
}

void searchMusic(sqlite3* db) {
    std::string term;
    readLine("Enter a song title or artist name: ", term);

    const char* sql =
        "SELECT Songs.title, Albums.title, Artists.name, Songs.duration "
        "FROM Songs "
        "JOIN Albums ON Songs.album_id = Albums.album_id "
        "JOIN Artists ON Albums.artist_id = Artists.artist_id "
        "WHERE Songs.title LIKE ? OR Artists.name LIKE ? "
        "ORDER BY Artists.name, Songs.title;";

    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (result != SQLITE_OK) {
        std::cerr << "Could not prepare search: " << sqlite3_errmsg(db) << '\n';
        return;
    }

    std::string pattern = "%" + term + "%";
    sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, pattern.c_str(), -1, SQLITE_TRANSIENT);

    bool found = false;
    while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
        found = true;
        std::cout << "Song: " << textColumn(stmt, 0)
                  << " | Album: " << textColumn(stmt, 1)
                  << " | Artist: " << textColumn(stmt, 2)
                  << " | Duration (seconds): " << sqlite3_column_int(stmt, 3)
                  << '\n';
    }

    if (!found && result == SQLITE_DONE) {
        std::cout << "No matching songs found.\n";
    } else if (result != SQLITE_DONE) {
        std::cerr << "Search failed: " << sqlite3_errmsg(db) << '\n';
    }

    sqlite3_finalize(stmt);
}

void addArtist(sqlite3* db) {
    std::string name;
    readLine("Enter the artist's name: ", name);

    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(
        db, "INSERT INTO Artists (name) VALUES (?);", -1, &stmt, nullptr);

    if (result == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
        result = sqlite3_step(stmt);
    }

    if (result == SQLITE_DONE) {
        std::cout << "Artist added successfully.\n";
    } else {
        std::cerr << "Could not add artist: " << sqlite3_errmsg(db) << '\n';
    }

    sqlite3_finalize(stmt);
}

void addAlbum(sqlite3* db) {
    std::string title;
    int artistId;

    readLine("Enter the album title: ", title);
    if (!readNumber("Enter the artist ID from option 1: ", artistId)) return;

    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(
        db, "INSERT INTO Albums (title, artist_id) VALUES (?, ?);",
        -1, &stmt, nullptr);

    if (result == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, artistId);
        result = sqlite3_step(stmt);
    }

    if (result == SQLITE_DONE) {
        std::cout << "Album added successfully.\n";
    } else {
        std::cerr << "Could not add album: " << sqlite3_errmsg(db) << '\n';
    }

    sqlite3_finalize(stmt);
}

void addSong(sqlite3* db) {
    std::string title;
    int duration;
    int albumId;

    readLine("Enter the song title: ", title);
    if (!readNumber("Enter duration in seconds: ", duration)) return;
    if (duration <= 0) {
        std::cout << "Duration must be positive.\n";
        return;
    }
    if (!readNumber("Enter the album ID from option 2: ", albumId)) return;

    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(
        db, "INSERT INTO Songs (title, duration, album_id) VALUES (?, ?, ?);",
        -1, &stmt, nullptr);

    if (result == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, duration);
        sqlite3_bind_int(stmt, 3, albumId);
        result = sqlite3_step(stmt);
    }

    if (result == SQLITE_DONE) {
        std::cout << "Song added successfully.\n";
    } else {
        std::cerr << "Could not add song: " << sqlite3_errmsg(db) << '\n';
    }

    sqlite3_finalize(stmt);
}

void updateSong(sqlite3* db) {
    int songId;
    int duration;
    std::string title;

    if (!readNumber("Enter the song ID to update: ", songId)) return;
    readLine("Enter the new song title: ", title);
    if (!readNumber("Enter the new duration in seconds: ", duration)) return;
    if (duration <= 0) {
        std::cout << "Duration must be positive.\n";
        return;
    }

    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(
        db, "UPDATE Songs SET title = ?, duration = ? WHERE song_id = ?;",
        -1, &stmt, nullptr);

    if (result == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, duration);
        sqlite3_bind_int(stmt, 3, songId);
        result = sqlite3_step(stmt);
    }

    if (result == SQLITE_DONE && sqlite3_changes(db) > 0) {
        std::cout << "Song updated successfully.\n";
    } else if (result == SQLITE_DONE) {
        std::cout << "No song with that ID was found.\n";
    } else {
        std::cerr << "Could not update song: " << sqlite3_errmsg(db) << '\n';
    }

    sqlite3_finalize(stmt);
}

void deleteSong(sqlite3* db) {
    int songId;
    if (!readNumber("Enter the song ID to delete: ", songId)) return;

    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(
        db, "DELETE FROM Songs WHERE song_id = ?;", -1, &stmt, nullptr);

    if (result == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, songId);
        result = sqlite3_step(stmt);
    }

    if (result == SQLITE_DONE && sqlite3_changes(db) > 0) {
        std::cout << "Song deleted successfully.\n";
    } else if (result == SQLITE_DONE) {
        std::cout << "No song with that ID was found.\n";
    } else {
        std::cerr << "Could not delete song: " << sqlite3_errmsg(db) << '\n'
                  << "It may be linked to a playlist, genre, or history.\n";
    }

    sqlite3_finalize(stmt);
}

void createPlaylist(sqlite3* db) {
    std::string name;
    readLine("Enter a name for the playlist: ", name);

    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(
        db, "INSERT INTO Playlists (name) VALUES (?);", -1, &stmt, nullptr);

    if (result == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
        result = sqlite3_step(stmt);
    }

    if (result == SQLITE_DONE) {
        std::cout << "Playlist created successfully.\n";
    } else {
        std::cerr << "Could not create playlist: " << sqlite3_errmsg(db) << '\n';
    }

    sqlite3_finalize(stmt);
}

void addSongToPlaylist(sqlite3* db) {
    int playlistId, songId, position;

    if (!readNumber("Enter the playlist ID from option 11: ", playlistId)) return;
    if (!readNumber("Enter the song ID from option 3: ", songId)) return;
    if (!readNumber("Enter its position in the playlist: ", position)) return;
    if (position <= 0) {
        std::cout << "Position must be positive.\n";
        return;
    }

    const char* sql =
        "INSERT INTO PlaylistSongs (playlist_id, song_id, position) "
        "VALUES (?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (result == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, playlistId);
        sqlite3_bind_int(stmt, 2, songId);
        sqlite3_bind_int(stmt, 3, position);
        result = sqlite3_step(stmt);
    }

    if (result == SQLITE_DONE) {
        std::cout << "Song added to playlist.\n";
    } else {
        std::cerr << "Could not add song: " << sqlite3_errmsg(db) << '\n'
                  << "Check the IDs and make sure the position is unused.\n";
    }

    sqlite3_finalize(stmt);
}

void viewPlaylistSongs(sqlite3* db) {
    int playlistId;
    if (!readNumber("Enter the playlist ID from option 11: ", playlistId)) return;

    const char* sql =
        "SELECT PlaylistSongs.position, Songs.song_id, Songs.title, Songs.duration "
        "FROM PlaylistSongs "
        "JOIN Songs ON PlaylistSongs.song_id = Songs.song_id "
        "WHERE PlaylistSongs.playlist_id = ? "
        "ORDER BY PlaylistSongs.position;";

    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (result != SQLITE_OK) {
        std::cerr << "Could not read playlist: " << sqlite3_errmsg(db) << '\n';
        return;
    }

    sqlite3_bind_int(stmt, 1, playlistId);
    bool found = false;

    while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
        found = true;
        std::cout << "Position: " << sqlite3_column_int(stmt, 0)
                  << " | Song ID: " << sqlite3_column_int(stmt, 1)
                  << " | Song: " << textColumn(stmt, 2)
                  << " | Duration: " << sqlite3_column_int(stmt, 3) << '\n';
    }

    if (!found && result == SQLITE_DONE) {
        std::cout << "No songs found for that playlist ID.\n";
    } else if (result != SQLITE_DONE) {
        std::cerr << "Could not read playlist: " << sqlite3_errmsg(db) << '\n';
    }

    sqlite3_finalize(stmt);
}

void recordListening(sqlite3* db) {
    int songId;
    if (!readNumber("Enter the song ID you listened to: ", songId)) return;

    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(
        db, "INSERT INTO ListeningHistory (song_id) VALUES (?);",
        -1, &stmt, nullptr);

    if (result == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, songId);
        result = sqlite3_step(stmt);
    }

    if (result == SQLITE_DONE) {
        std::cout << "Listening event recorded.\n";
    } else {
        std::cerr << "Could not record listening event: "
                  << sqlite3_errmsg(db) << '\n';
    }

    sqlite3_finalize(stmt);
}

void addOsJob(sqlite3* db) {
    std::string jobName;
    int songId;
    int arrivalTime;
    int burstTime;
    int memoryRequired;

    readLine("Describe the job (for example, Scan library): ", jobName);

    if (!readNumber("Song ID, or 0 if this is a general library job: ", songId))
        return;
    if (!readNumber("Arrival time (0 or more): ", arrivalTime))
        return;
    if (!readNumber("CPU burst time (must be above 0): ", burstTime))
        return;
    if (!readNumber("Memory required (must be above 0): ", memoryRequired))
        return;

    if (songId < 0 || arrivalTime < 0 || burstTime <= 0 || memoryRequired <= 0) {
        std::cout << "Arrival time must be 0 or more; burst and memory must be above 0.\n";
        return;
    }

    const char* sql =
        "INSERT INTO OSJobs "
        "(job_name, song_id, arrival_time, burst_time, memory_required) "
        "VALUES (?, NULLIF(?, 0), ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (result == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, jobName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, songId);
        sqlite3_bind_int(stmt, 3, arrivalTime);
        sqlite3_bind_int(stmt, 4, burstTime);
        sqlite3_bind_int(stmt, 5, memoryRequired);
        result = sqlite3_step(stmt);
    }

    if (result == SQLITE_DONE) {
        std::cout << "OS job saved in the database.\n";
    } else {
        std::cerr << "Could not save OS job: " << sqlite3_errmsg(db) << '\n';
    }

    sqlite3_finalize(stmt);
}

int main() {
    sqlite3* database = nullptr;
    const char* databasePath =
        "D:/PBL DBMS & OS/Museos/database/museos.db";

    int result = sqlite3_open(databasePath, &database);

    if (result != SQLITE_OK) {
        std::cerr << "Could not open the database: "
                  << (database ? sqlite3_errmsg(database) : "unknown error")
                  << '\n';
        if (database) sqlite3_close(database);
        return 1;
    }

    result = sqlite3_exec(
        database, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);

    if (result != SQLITE_OK) {
        std::cerr << "Could not enable relationship checks: "
                  << sqlite3_errmsg(database) << '\n';
        sqlite3_close(database);
        return 1;
    }

    int choice = -1;

    while (choice != 0) {
        std::cout << "\n=== MuseOS ===\n"
                  << "1. View all artists\n"
                  << "2. View all albums\n"
                  << "3. View all songs\n"
                  << "4. Search songs or artists\n"
                  << "5. Add an artist\n"
                  << "6. Add an album\n"
                  << "7. Add a song\n"
                  << "8. Update a song\n"
                  << "9. Delete a song\n"
                  << "10. Create a playlist\n"
                  << "11. View playlists\n"
                  << "12. Add a song to a playlist\n"
                  << "13. View songs in a playlist\n"
                  << "14. Record a song as listened to\n"
                  << "15. View listening history\n"
                  << "16. Show most-played songs\n"
                  << "17. Add an OS workload job\n"
                  << "18. View waiting OS jobs\n"
                  << "0. Exit\n"
                  << "Choose an option: ";

        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Please enter a menu number.\n";
            continue;
        }

        const char* query = nullptr;

        if (choice == 1) {
            query = "SELECT artist_id, name FROM Artists ORDER BY name;";
        } else if (choice == 2) {
            query =
                "SELECT Albums.album_id, Albums.title, Artists.name AS artist "
                "FROM Albums JOIN Artists "
                "ON Albums.artist_id = Artists.artist_id "
                "ORDER BY Artists.name, Albums.title;";
        } else if (choice == 3) {
            query =
                "SELECT Songs.song_id, Songs.title, Albums.title AS album, "
                "Artists.name AS artist, Songs.duration "
                "FROM Songs "
                "JOIN Albums ON Songs.album_id = Albums.album_id "
                "JOIN Artists ON Albums.artist_id = Artists.artist_id "
                "ORDER BY Artists.name, Albums.title, Songs.title;";
        } else if (choice == 4) {
            searchMusic(database);
        } else if (choice == 5) {
            addArtist(database);
        } else if (choice == 6) {
            addAlbum(database);
        } else if (choice == 7) {
            addSong(database);
        } else if (choice == 8) {
            updateSong(database);
        } else if (choice == 9) {
            deleteSong(database);
        } else if (choice == 10) {
            createPlaylist(database);
        } else if (choice == 11) {
            query = "SELECT playlist_id, name, created_at FROM Playlists ORDER BY name;";
        } else if (choice == 12) {
            addSongToPlaylist(database);
        } else if (choice == 13) {
            viewPlaylistSongs(database);
        } else if (choice == 14) {
            recordListening(database);
        } else if (choice == 15) {
            query =
                "SELECT ListeningHistory.history_id, Songs.title, "
                "ListeningHistory.listened_at "
                "FROM ListeningHistory "
                "JOIN Songs ON ListeningHistory.song_id = Songs.song_id "
                "ORDER BY ListeningHistory.listened_at DESC;";
        } else if (choice == 16) {
            query =
                "SELECT Songs.title, Artists.name AS artist, "
                "COUNT(ListeningHistory.history_id) AS play_count "
                "FROM ListeningHistory "
                "JOIN Songs ON ListeningHistory.song_id = Songs.song_id "
                "JOIN Albums ON Songs.album_id = Albums.album_id "
                "JOIN Artists ON Albums.artist_id = Artists.artist_id "
                "GROUP BY Songs.song_id "
                "ORDER BY play_count DESC, Songs.title;";
                } else if (choice == 17) {
    addOsJob(database);
} else if (choice == 18) {
    query =
        "SELECT job_id, job_name, song_id, arrival_time, burst_time, "
        "memory_required, status "
        "FROM OSJobs "
        "WHERE status = 'READY' "
        "ORDER BY arrival_time, job_id;";
        } else if (choice != 0) {
            std::cout << "That menu option is not available.\n";
        }

        if (query != nullptr) {
            runQuery(database, query);
        }
    }

    sqlite3_close(database);
    std::cout << "Goodbye!\n";
    return 0;
}