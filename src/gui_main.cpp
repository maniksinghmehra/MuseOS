#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQueryModel>
#include <QTabWidget>
#include <QTableView>
#include <QVBoxLayout>
#include <QWidget>
#include <QAbstractItemView>
#include <QLineEdit>
#include <QSortFilterProxyModel>
#include <QInputDialog>
#include <QMessageBox>
#include <QSqlQuery>
#include <QList>
#include <QStringList>
#include <QTextStream>

QWidget* createTablePage(
    const QString& query,
    const QStringList& headers,
    const QSqlDatabase& database,
    const QString& addButtonText = QString(),
    const QString& dialogTitle = QString(),
    const QString& fieldPrompt = QString(),
    const QString& insertSql = QString(),
    bool addAlbum = false,
    bool addSong = false,
    bool editSong = false,
    bool deleteSong = false,
    bool addPlaylist = false,
    bool addToPlaylist = false,
    bool viewPlaylistSongs = false,
    bool recordListen = false,
    bool assignGenre = false,
    bool exportSongs = false
) {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);

    auto* status = new QLabel;
    auto* searchBox = new QLineEdit;
    searchBox->setPlaceholderText("Search this list...");

    auto* refreshButton = new QPushButton("Refresh");
    auto* table = new QTableView;

    auto* model = new QSqlQueryModel(table);
    model->setQuery(query, database);

    for (int column = 0; column < headers.size(); ++column) {
        model->setHeaderData(column, Qt::Horizontal, headers[column]);
    }

    auto* proxyModel = new QSortFilterProxyModel(page);
    proxyModel->setSourceModel(model);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterKeyColumn(-1);

    table->setModel(proxyModel);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->verticalHeader()->hide();
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    auto reloadTable = [model, query, database, status]() {
        model->setQuery(query, database);

        if (model->lastError().isValid()) {
            status->setText("Database error: " + model->lastError().text());
        } else {
            status->setText(
                QString("%1 records").arg(model->rowCount())
            );
        }
    };

    reloadTable();

    QObject::connect(
        searchBox,
        &QLineEdit::textChanged,
        proxyModel,
        [proxyModel](const QString& text) {
            proxyModel->setFilterFixedString(text);
        }
    );

    QObject::connect(
        refreshButton,
        &QPushButton::clicked,
        reloadTable
    );

    layout->addWidget(status);
    layout->addWidget(searchBox);
    layout->addWidget(refreshButton);

    if (!addButtonText.isEmpty()) {
        auto* addButton = new QPushButton(addButtonText);
        layout->addWidget(addButton);

                QObject::connect(
            addButton,
            &QPushButton::clicked,
            page,
            [model, database, page, dialogTitle, fieldPrompt,
             insertSql, reloadTable, addAlbum, addSong, addPlaylist]() {
                if (addAlbum) {
                    QSqlQuery artistQuery(database);

                    if (!artistQuery.exec(
                            "SELECT artist_id, name "
                            "FROM Artists ORDER BY name;")) {
                        QMessageBox::warning(
                            page,
                            "Could not load artists",
                            artistQuery.lastError().text()
                        );
                        return;
                    }

                    QStringList artistNames;
                    QList<int> artistIds;

                    while (artistQuery.next()) {
                        artistIds.append(artistQuery.value(0).toInt());
                        artistNames.append(artistQuery.value(1).toString());
                    }

                    if (artistNames.isEmpty()) {
                        QMessageBox::warning(
                            page,
                            "No artists",
                            "Add an artist before adding an album."
                        );
                        return;
                    }

                    bool titleAccepted = false;
                    const QString albumTitle = QInputDialog::getText(
                        page,
                        dialogTitle,
                        fieldPrompt,
                        QLineEdit::Normal,
                        QString(),
                        &titleAccepted
                    ).trimmed();

                    if (!titleAccepted) {
                        return;
                    }

                    if (albumTitle.isEmpty()) {
                        QMessageBox::warning(
                            page,
                            "Missing album title",
                            "Please enter an album title."
                        );
                        return;
                    }

                    bool artistAccepted = false;
                    const QString artistName = QInputDialog::getItem(
                        page,
                        "Choose artist",
                        "Artist:",
                        artistNames,
                        0,
                        false,
                        &artistAccepted
                    );

                    if (!artistAccepted) {
                        return;
                    }

                    const int artistIndex = artistNames.indexOf(artistName);
                    if (artistIndex < 0) {
                        return;
                    }

                    QSqlQuery insert(database);
                    insert.prepare(
                        "INSERT INTO Albums (title, artist_id) "
                        "VALUES (?, ?);"
                    );
                    insert.addBindValue(albumTitle);
                    insert.addBindValue(artistIds[artistIndex]);

                    if (!insert.exec()) {
                        QMessageBox::warning(
                            page,
                            "Could not add album",
                            insert.lastError().text()
                        );
                        return;
                    }

                    reloadTable();
                    return;
                }
                                if (addSong) {
                    QSqlQuery albumQuery(database);

                    if (!albumQuery.exec(
                            "SELECT Albums.album_id, Albums.title, Artists.name "
                            "FROM Albums "
                            "JOIN Artists ON Albums.artist_id = Artists.artist_id "
                            "ORDER BY Albums.title;")) {
                        QMessageBox::warning(
                            page,
                            "Could not load albums",
                            albumQuery.lastError().text()
                        );
                        return;
                    }

                    QStringList albumOptions;
                    QList<int> albumIds;

                    while (albumQuery.next()) {
                        albumIds.append(albumQuery.value(0).toInt());
                        albumOptions.append(
                            albumQuery.value(1).toString() + " - " +
                            albumQuery.value(2).toString()
                        );
                    }

                    if (albumOptions.isEmpty()) {
                        QMessageBox::warning(
                            page,
                            "No albums",
                            "Add an album before adding a song."
                        );
                        return;
                    }

                    bool titleAccepted = false;
                    const QString songTitle = QInputDialog::getText(
                        page,
                        "Add song",
                        "Song title:",
                        QLineEdit::Normal,
                        QString(),
                        &titleAccepted
                    ).trimmed();

                    if (!titleAccepted) {
                        return;
                    }

                    if (songTitle.isEmpty()) {
                        QMessageBox::warning(
                            page,
                            "Missing title",
                            "Please enter a song title."
                        );
                        return;
                    }

                    bool albumAccepted = false;
                    const QString albumChoice = QInputDialog::getItem(
                        page,
                        "Choose album",
                        "Album:",
                        albumOptions,
                        0,
                        false,
                        &albumAccepted
                    );

                    if (!albumAccepted) {
                        return;
                    }

                    const int albumIndex = albumOptions.indexOf(albumChoice);
                    if (albumIndex < 0) {
                        return;
                    }

                    bool durationAccepted = false;
                    const int durationSeconds = QInputDialog::getInt(
                        page,
                        "Song duration",
                        "Duration in seconds (314 means 5:14):",
                        180,
                        1,
                        86400,
                        1,
                        &durationAccepted
                    );

                    if (!durationAccepted) {
                        return;
                    }

                    QSqlQuery insert(database);
                    insert.prepare(
                        "INSERT INTO Songs (title, album_id, duration) "
                        "VALUES (?, ?, ?);"
                    );
                    insert.addBindValue(songTitle);
                    insert.addBindValue(albumIds[albumIndex]);
                    insert.addBindValue(durationSeconds);

                    if (!insert.exec()) {
                        QMessageBox::warning(
                            page,
                            "Could not add song",
                            insert.lastError().text()
                        );
                        return;
                    }

                    reloadTable();
                    return;
                }

                if (addPlaylist) {
                    bool accepted = false;
                    const QString playlistName = QInputDialog::getText(
                        page,
                        "Create playlist",
                        "Playlist name:",
                        QLineEdit::Normal,
                        QString(),
                        &accepted
                    ).trimmed();

                    if (!accepted) {
                        return;
                    }

                    if (playlistName.isEmpty()) {
                        QMessageBox::warning(
                            page,
                            "Missing name",
                            "Please enter a playlist name."
                        );
                        return;
                    }

                    QSqlQuery insert(database);
                    insert.prepare(
                        "INSERT INTO Playlists (name) VALUES (?);"
                    );
                    insert.addBindValue(playlistName);

                    if (!insert.exec()) {
                        QMessageBox::warning(
                            page,
                            "Could not create playlist",
                            insert.lastError().text()
                        );
                        return;
                    }

                    reloadTable();
                    return;
                }

                bool accepted = false;
                const QString name = QInputDialog::getText(
                    page,
                    dialogTitle,
                    fieldPrompt,
                    QLineEdit::Normal,
                    QString(),
                    &accepted
                ).trimmed();

                if (!accepted) {
                    return;
                }

                if (name.isEmpty()) {
                    QMessageBox::warning(
                        page,
                        "Missing name",
                        "Please enter an artist name."
                    );
                    return;
                }

                QSqlQuery insert(database);
                insert.prepare(insertSql);
                insert.addBindValue(name);

                if (!insert.exec()) {
                    QMessageBox::warning(
                        page,
                        "Could not add artist",
                        insert.lastError().text()
                    );
                    return;
                }

                reloadTable();
            }
        );
    }

    if (editSong) {
        auto* editButton = new QPushButton("Edit selected song");
        layout->addWidget(editButton);

        QObject::connect(
            editButton,
            &QPushButton::clicked,
            page,
            [table, proxyModel, model, database, page, reloadTable]() {
                const QModelIndex selected = table->currentIndex();

                if (!selected.isValid()) {
                    QMessageBox::information(
                        page,
                        "Select a song",
                        "Click a song row first."
                    );
                    return;
                }

                const QModelIndex source =
                    proxyModel->mapToSource(selected);
                const int songId =
                    model->data(model->index(source.row(), 0)).toInt();

                QSqlQuery currentSong(database);
                currentSong.prepare(
                    "SELECT title, duration FROM Songs WHERE song_id = ?;"
                );
                currentSong.addBindValue(songId);

                if (!currentSong.exec() || !currentSong.next()) {
                    QMessageBox::warning(
                        page,
                        "Could not load song",
                        currentSong.lastError().text()
                    );
                    return;
                }

                const QString oldTitle =
                    currentSong.value(0).toString();
                const int oldDuration =
                    currentSong.value(1).toInt();

                bool titleAccepted = false;
                const QString newTitle = QInputDialog::getText(
                    page,
                    "Edit song",
                    "Song title:",
                    QLineEdit::Normal,
                    oldTitle,
                    &titleAccepted
                ).trimmed();

                if (!titleAccepted) {
                    return;
                }

                if (newTitle.isEmpty()) {
                    QMessageBox::warning(
                        page,
                        "Missing title",
                        "Song title cannot be empty."
                    );
                    return;
                }

                bool durationAccepted = false;
                const int newDuration = QInputDialog::getInt(
                    page,
                    "Edit song",
                    "Duration in seconds:",
                    oldDuration,
                    1,
                    86400,
                    1,
                    &durationAccepted
                );

                if (!durationAccepted) {
                    return;
                }

                QSqlQuery update(database);
                update.prepare(
                    "UPDATE Songs SET title = ?, duration = ? "
                    "WHERE song_id = ?;"
                );
                update.addBindValue(newTitle);
                update.addBindValue(newDuration);
                update.addBindValue(songId);

                if (!update.exec()) {
                    QMessageBox::warning(
                        page,
                        "Could not update song",
                        update.lastError().text()
                    );
                    return;
                }

                reloadTable();

                QMessageBox::information(
                    page,
                    "Song updated",
                    "The song was updated successfully."
                );
            }
        );
    }

    if (deleteSong) {
        auto* deleteButton = new QPushButton("Delete selected song");
        layout->addWidget(deleteButton);

        QObject::connect(
            deleteButton,
            &QPushButton::clicked,
            page,
            [table, proxyModel, model, database, page, reloadTable]() {
                const QModelIndex selected = table->currentIndex();

                if (!selected.isValid()) {
                    QMessageBox::information(
                        page,
                        "Select a song",
                        "Click a song row first."
                    );
                    return;
                }

                const QModelIndex source =
                    proxyModel->mapToSource(selected);
                const int songId =
                    model->data(model->index(source.row(), 0)).toInt();
                const QString songTitle =
                    model->data(model->index(source.row(), 1)).toString();

                const auto answer = QMessageBox::question(
                    page,
                    "Confirm delete",
                    "Delete \"" + songTitle + "\"?",
                    QMessageBox::Yes | QMessageBox::No
                );

                if (answer != QMessageBox::Yes) {
                    return;
                }

                QSqlQuery remove(database);
                remove.prepare("DELETE FROM Songs WHERE song_id = ?;");
                remove.addBindValue(songId);

                if (!remove.exec()) {
                    QMessageBox::warning(
                        page,
                        "Could not delete song",
                        "The song may still be linked to a playlist, genre, "
                        "or listening history.\n\n" +
                        remove.lastError().text()
                    );
                    return;
                }

                reloadTable();
            }
        );
    }

    if (addToPlaylist) {
        auto* addSongButton =
            new QPushButton("Add song to selected playlist");
        layout->addWidget(addSongButton);

        QObject::connect(
            addSongButton,
            &QPushButton::clicked,
            page,
            [table, proxyModel, model, database, page, reloadTable]() {
                const QModelIndex selected = table->currentIndex();

                if (!selected.isValid()) {
                    QMessageBox::information(
                        page,
                        "Select a playlist",
                        "Click a playlist row first."
                    );
                    return;
                }

                const QModelIndex source =
                    proxyModel->mapToSource(selected);
                const int playlistId =
                    model->data(model->index(source.row(), 0)).toInt();

                QSqlQuery songsQuery(database);

                if (!songsQuery.exec(
                        "SELECT Songs.song_id, Songs.title, Artists.name "
                        "FROM Songs "
                        "JOIN Albums ON Songs.album_id = Albums.album_id "
                        "JOIN Artists ON Albums.artist_id = Artists.artist_id "
                        "ORDER BY Songs.title;")) {
                    QMessageBox::warning(
                        page,
                        "Could not load songs",
                        songsQuery.lastError().text()
                    );
                    return;
                }

                QStringList songChoices;
                QList<int> songIds;

                while (songsQuery.next()) {
                    songIds.append(songsQuery.value(0).toInt());
                    songChoices.append(
                        songsQuery.value(1).toString() + " - " +
                        songsQuery.value(2).toString()
                    );
                }

                if (songChoices.isEmpty()) {
                    QMessageBox::information(
                        page,
                        "No songs",
                        "Add songs to the library first."
                    );
                    return;
                }

                bool accepted = false;
                const QString songChoice = QInputDialog::getItem(
                    page,
                    "Add song to playlist",
                    "Choose a song:",
                    songChoices,
                    0,
                    false,
                    &accepted
                );

                if (!accepted) {
                    return;
                }

                const int songIndex = songChoices.indexOf(songChoice);
                if (songIndex < 0) {
                    return;
                }

                QSqlQuery positionQuery(database);
                positionQuery.prepare(
                    "SELECT COALESCE(MAX(position), 0) + 1 "
                    "FROM PlaylistSongs WHERE playlist_id = ?;"
                );
                positionQuery.addBindValue(playlistId);

                if (!positionQuery.exec() || !positionQuery.next()) {
                    QMessageBox::warning(
                        page,
                        "Could not find playlist position",
                        positionQuery.lastError().text()
                    );
                    return;
                }

                const int nextPosition =
                    positionQuery.value(0).toInt();

                QSqlQuery insert(database);
                insert.prepare(
                    "INSERT INTO PlaylistSongs "
                    "(playlist_id, song_id, position) VALUES (?, ?, ?);"
                );
                insert.addBindValue(playlistId);
                insert.addBindValue(songIds[songIndex]);
                insert.addBindValue(nextPosition);

                if (!insert.exec()) {
                    QMessageBox::warning(
                        page,
                        "Could not add song",
                        insert.lastError().text()
                    );
                    return;
                }

                reloadTable();

                QMessageBox::information(
                    page,
                    "Song added",
                    "The song was added to the playlist."
                );
            }
        );
    }

    if (viewPlaylistSongs) {
        auto* viewButton =
            new QPushButton("View songs in selected playlist");
        layout->addWidget(viewButton);

        QObject::connect(
            viewButton,
            &QPushButton::clicked,
            page,
            [table, proxyModel, model, database, page]() {
                const QModelIndex selected = table->currentIndex();

                if (!selected.isValid()) {
                    QMessageBox::information(
                        page,
                        "Select a playlist",
                        "Click a playlist row first."
                    );
                    return;
                }

                const QModelIndex source =
                    proxyModel->mapToSource(selected);
                const int playlistId =
                    model->data(model->index(source.row(), 0)).toInt();
                const QString playlistName =
                    model->data(model->index(source.row(), 1)).toString();

                QSqlQuery songsQuery(database);
                songsQuery.prepare(
                    "SELECT PlaylistSongs.position, Songs.title, "
                    "Artists.name, "
                    "printf('%d:%02d', Songs.duration / 60, "
                    "Songs.duration % 60) "
                    "FROM PlaylistSongs "
                    "JOIN Songs ON PlaylistSongs.song_id = Songs.song_id "
                    "JOIN Albums ON Songs.album_id = Albums.album_id "
                    "JOIN Artists ON Albums.artist_id = Artists.artist_id "
                    "WHERE PlaylistSongs.playlist_id = ? "
                    "ORDER BY PlaylistSongs.position;"
                );
                songsQuery.addBindValue(playlistId);

                if (!songsQuery.exec()) {
                    QMessageBox::warning(
                        page,
                        "Could not load playlist songs",
                        songsQuery.lastError().text()
                    );
                    return;
                }

                auto* dialog = new QDialog(page);
                dialog->setWindowTitle("Playlist: " + playlistName);
                dialog->resize(700, 400);

                auto* dialogLayout = new QVBoxLayout(dialog);
                auto* songModel = new QSqlQueryModel(dialog);
                songModel->setQuery(songsQuery);

                songModel->setHeaderData(
                    0, Qt::Horizontal, "Position"
                );
                songModel->setHeaderData(
                    1, Qt::Horizontal, "Song"
                );
                songModel->setHeaderData(
                    2, Qt::Horizontal, "Artist"
                );
                songModel->setHeaderData(
                    3, Qt::Horizontal, "Duration"
                );

                auto* songTable = new QTableView(dialog);
                songTable->setModel(songModel);
                songTable->verticalHeader()->hide();
                songTable->horizontalHeader()->setSectionResizeMode(
                    QHeaderView::Stretch
                );

                auto* closeButton = new QPushButton("Close", dialog);
                QObject::connect(
                    closeButton,
                    &QPushButton::clicked,
                    dialog,
                    &QDialog::accept
                );

                dialogLayout->addWidget(songTable);
                dialogLayout->addWidget(closeButton);
                dialog->exec();
                dialog->deleteLater();
            }
        );
    }

    if (recordListen) {
        auto* listenButton =
            new QPushButton("Record selected song as listened");
        layout->addWidget(listenButton);

        QObject::connect(
            listenButton,
            &QPushButton::clicked,
            page,
            [table, proxyModel, model, database, page]() {
                const QModelIndex selected = table->currentIndex();

                if (!selected.isValid()) {
                    QMessageBox::information(
                        page,
                        "Select a song",
                        "Click a song row first."
                    );
                    return;
                }

                const QModelIndex source =
                    proxyModel->mapToSource(selected);
                const int songId =
                    model->data(model->index(source.row(), 0)).toInt();

                QSqlQuery insert(database);
                insert.prepare(
                    "INSERT INTO ListeningHistory (song_id) VALUES (?);"
                );
                insert.addBindValue(songId);

                if (!insert.exec()) {
                    QMessageBox::warning(
                        page,
                        "Could not record listen",
                        insert.lastError().text()
                    );
                    return;
                }

                QMessageBox::information(
                    page,
                    "Listen recorded",
                    "This song was added to your listening history."
                );
            }
        );
    }

    if (assignGenre) {
        auto* genreButton =
            new QPushButton("Assign genre to selected song");
        layout->addWidget(genreButton);

        QObject::connect(
            genreButton,
            &QPushButton::clicked,
            page,
            [table, proxyModel, model, database, page]() {
                const QModelIndex selected = table->currentIndex();

                if (!selected.isValid()) {
                    QMessageBox::information(
                        page,
                        "Select a song",
                        "Click a song row first."
                    );
                    return;
                }

                const QModelIndex source =
                    proxyModel->mapToSource(selected);
                const int songId =
                    model->data(model->index(source.row(), 0)).toInt();

                QSqlQuery genresQuery(database);

                if (!genresQuery.exec(
                        "SELECT genre_id, name FROM Genres ORDER BY name;")) {
                    QMessageBox::warning(
                        page,
                        "Could not load genres",
                        genresQuery.lastError().text()
                    );
                    return;
                }

                QStringList genreNames;
                QList<int> genreIds;

                while (genresQuery.next()) {
                    genreIds.append(genresQuery.value(0).toInt());
                    genreNames.append(genresQuery.value(1).toString());
                }

                if (genreNames.isEmpty()) {
                    QMessageBox::information(
                        page,
                        "No genres",
                        "There are no genres in the database yet."
                    );
                    return;
                }

                bool accepted = false;
                const QString genreName = QInputDialog::getItem(
                    page,
                    "Assign genre",
                    "Choose a genre:",
                    genreNames,
                    0,
                    false,
                    &accepted
                );

                if (!accepted) {
                    return;
                }

                const int genreIndex = genreNames.indexOf(genreName);
                if (genreIndex < 0) {
                    return;
                }

                QSqlQuery insert(database);
                insert.prepare(
                    "INSERT INTO SongGenres (song_id, genre_id) "
                    "VALUES (?, ?);"
                );
                insert.addBindValue(songId);
                insert.addBindValue(genreIds[genreIndex]);

                if (!insert.exec()) {
                    QMessageBox::warning(
                        page,
                        "Could not assign genre",
                        insert.lastError().text()
                    );
                    return;
                }

                QMessageBox::information(
                    page,
                    "Genre assigned",
                    "The genre was assigned to the song."
                );
            }
        );
    }

    if (exportSongs) {
        auto* exportButton = new QPushButton("Export songs to CSV");
        layout->addWidget(exportButton);

        QObject::connect(
            exportButton,
            &QPushButton::clicked,
            page,
            [database, page]() {
                const QString fileName = QFileDialog::getSaveFileName(
                    page,
                    "Export songs",
                    "museos_songs.csv",
                    "CSV files (*.csv)"
                );

                if (fileName.isEmpty()) {
                    return;
                }

                QSqlQuery exportQuery(database);
                if (!exportQuery.exec(
                        "SELECT Songs.song_id, Songs.title, Albums.title, "
                        "Artists.name, Songs.duration "
                        "FROM Songs "
                        "JOIN Albums ON Songs.album_id = Albums.album_id "
                        "JOIN Artists ON Albums.artist_id = Artists.artist_id "
                        "ORDER BY Artists.name, Albums.title, Songs.title;")) {
                    QMessageBox::warning(
                        page,
                        "Export failed",
                        exportQuery.lastError().text()
                    );
                    return;
                }

                QFile file(fileName);
                if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QMessageBox::warning(
                        page,
                        "Could not create CSV",
                        file.errorString()
                    );
                    return;
                }

                QTextStream output(&file);
                output << "song_id,title,album,artist,duration_seconds\n";

                auto quoteCsv = [](QString value) {
                    value.replace("\"", "\"\"");
                    return QString("\"") + value + QString("\"");
                };

                int exportedCount = 0;

                while (exportQuery.next()) {
                    output << exportQuery.value(0).toString() << ","
                           << quoteCsv(exportQuery.value(1).toString()) << ","
                           << quoteCsv(exportQuery.value(2).toString()) << ","
                           << quoteCsv(exportQuery.value(3).toString()) << ","
                           << exportQuery.value(4).toString() << "\n";
                    ++exportedCount;
                }

                file.close();

                QMessageBox::information(
                    page,
                    "Export complete",
                    QString("Exported %1 songs to:\n%2")
                        .arg(exportedCount)
                        .arg(fileName)
                );
            }
        );
    }

    layout->addWidget(table);
    return page;
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    const QString databasePath =
        QDir::current().filePath("database/museos.db");

    QSqlDatabase database = QSqlDatabase::addDatabase("QSQLITE");
    database.setDatabaseName(databasePath);

    if (!database.open()) {
        QLabel error(
            "Could not open the MuseOS database:\n" +
            database.lastError().text()
        );
        error.resize(500, 120);
        error.show();
        return app.exec();
    }

    QSqlQuery enableForeignKeys(database);
    if (!enableForeignKeys.exec("PRAGMA foreign_keys = ON;")) {
        QMessageBox::critical(
            nullptr,
            "Database error",
            enableForeignKeys.lastError().text()
        );
        return 1;
    }

    QWidget window;
    window.setWindowTitle("MuseOS — Music Library");
    window.resize(1000, 650);

    auto* layout = new QVBoxLayout(&window);

    auto* title = new QLabel("MuseOS");
    title->setStyleSheet("font-size: 28px; font-weight: bold;");

    auto* subtitle = new QLabel(
        "Music Library  |  Database connected"
    );

    auto* tabs = new QTabWidget;

        tabs->addTab(
        createTablePage(
            "SELECT artist_id, name FROM Artists ORDER BY name;",
            {"ID", "Artist"},
            database,
            "Add artist",
            "Add artist",
            "Artist name:",
            "INSERT INTO Artists (name) VALUES (?);"
        ),
        "Artists"
    );

        tabs->addTab(
        createTablePage(
            "SELECT Albums.album_id, Albums.title, Artists.name "
            "FROM Albums "
            "JOIN Artists ON Albums.artist_id = Artists.artist_id "
            "ORDER BY Albums.title;",
            {"ID", "Album", "Artist"},
            database,
            "Add album",
            "Add album",
            "Album title:",
            QString(),
            true
        ),
        "Albums"
    );

        tabs->addTab(
        createTablePage(
            "SELECT Songs.song_id, Songs.title, Albums.title, "
            "Artists.name, "
            "printf('%d:%02d', Songs.duration / 60, Songs.duration % 60) "
            "FROM Songs "
            "JOIN Albums ON Songs.album_id = Albums.album_id "
            "JOIN Artists ON Albums.artist_id = Artists.artist_id "
            "ORDER BY Artists.name, Albums.title, Songs.title;",
            {"ID", "Song", "Album", "Artist", "Duration"},
            database,
            "Add song",
            "Add song",
            "Song title:",
            QString(),
            false,
            true,
            true,
            true,
            false,
            false,
            false,
            true,
            true,
            true
        ),
        "Songs"
    );

    tabs->addTab(
        createTablePage(
            "SELECT playlist_id, name, created_at "
            "FROM Playlists ORDER BY name;",
            {"ID", "Playlist", "Created"},
            database,
            "Create playlist",
            "Create playlist",
            "Playlist name:",
            QString(),
            false,
            false,
            false,
            false,
            true,
            true,
            true
        ),
        "Playlists"
    );

    tabs->addTab(
        createTablePage(
            "SELECT ListeningHistory.history_id, Songs.title, "
            "Artists.name, ListeningHistory.listened_at "
            "FROM ListeningHistory "
            "JOIN Songs ON ListeningHistory.song_id = Songs.song_id "
            "JOIN Albums ON Songs.album_id = Albums.album_id "
            "JOIN Artists ON Albums.artist_id = Artists.artist_id "
            "ORDER BY ListeningHistory.listened_at DESC;",
            {"ID", "Song", "Artist", "Listened at"},
            database
        ),
        "Listening History"
    );

    tabs->addTab(
        createTablePage(
            "SELECT Songs.title, Artists.name, "
            "COUNT(ListeningHistory.history_id) AS play_count "
            "FROM Songs "
            "JOIN Albums ON Songs.album_id = Albums.album_id "
            "JOIN Artists ON Albums.artist_id = Artists.artist_id "
            "LEFT JOIN ListeningHistory "
            "ON Songs.song_id = ListeningHistory.song_id "
            "GROUP BY Songs.song_id "
            "ORDER BY play_count DESC, Songs.title;",
            {"Song", "Artist", "Play count"},
            database
        ),
        "Most Played"
    );

    tabs->addTab(
        createTablePage(
            "SELECT Genres.genre_id, Genres.name, "
            "COUNT(SongGenres.song_id) AS song_count "
            "FROM Genres "
            "LEFT JOIN SongGenres "
            "ON Genres.genre_id = SongGenres.genre_id "
            "GROUP BY Genres.genre_id, Genres.name "
            "ORDER BY Genres.name;",
            {"ID", "Genre", "Song count"},
            database
        ),
        "Genres"
    );

    tabs->addTab(
        createTablePage(
            "SELECT 'Artists' AS item, COUNT(*) AS total FROM Artists "
            "UNION ALL "
            "SELECT 'Albums', COUNT(*) FROM Albums "
            "UNION ALL "
            "SELECT 'Songs', COUNT(*) FROM Songs "
            "UNION ALL "
            "SELECT 'Genres', COUNT(*) FROM Genres "
            "UNION ALL "
            "SELECT 'Playlists', COUNT(*) FROM Playlists "
            "UNION ALL "
            "SELECT 'Listening events', COUNT(*) FROM ListeningHistory;",
            {"Library item", "Count"},
            database
        ),
        "Dashboard"
    );

    tabs->addTab(
        createTablePage(
            "SELECT 'Database integrity' AS check_name, "
            "integrity_check AS result FROM pragma_integrity_check "
            "UNION ALL "
            "SELECT 'Foreign key violations', "
            "CAST(COUNT(*) AS TEXT) || ' row(s)' "
            "FROM pragma_foreign_key_check;",
            {"Check", "Result"},
            database
        ),
        "Database Health"
    );

    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addWidget(tabs);

    window.show();
    return app.exec();
}