#pragma once

#include <QString>
#include <QWidget>

class QLineEdit;
class QTreeWidget;
class QTreeWidgetItem;

/**
 * @brief Панель отображения импортированных ресурсов проекта.
 *
 * ProjectPanel представляет ресурсы в виде дерева Assets и предоставляет
 * строку поиска по импортированным моделям. Панель не загружает файлы и не
 * владеет ресурсами движка: она хранит только абсолютные пути в Qt::UserRole.
 *
 * В текущей реализации дерево содержит категории:
 * - Models;
 * - Textures;
 * - Shaders;
 * - Materials.
 *
 * Фактическое динамическое наполнение реализовано только для Models через
 * AddImportedFile(). Остальные категории являются структурными элементами UI.
 */
class ProjectPanel final : public QWidget {
public:
    /**
     * @brief Создаёт панель ресурсов проекта.
     * @param parent Родительский QWidget.
     */
    explicit ProjectPanel(QWidget* parent = nullptr);

    /**
     * @brief Добавляет импортированный файл в раздел Models.
     *
     * Один и тот же абсолютный путь не добавляется повторно. При повторном
     * вызове уже существующий элемент просто становится текущим.
     *
     * @param file_path Путь к импортированному файлу.
     */
    void AddImportedFile(const QString& file_path);

    /**
     * @brief Удаляет из дерева все ранее добавленные модели.
     *
     * Структура Assets и категорий при этом сохраняется.
     */
    void ClearImportedFiles();

private:
    /**
     * @brief Создаёт layout, строку поиска и дерево ресурсов.
     */
    void CreateLayout();

    /**
     * @brief Создаёт статическую структуру Assets.
     */
    void CreateTree();

    /**
     * @brief Фильтрует импортированные модели по имени файла.
     * @param text Текст из поисковой строки.
     */
    void FilterTree(const QString& text);

private:
    QLineEdit* search_line_ = nullptr;
    QTreeWidget* tree_ = nullptr;

    QTreeWidgetItem* assets_item_ = nullptr;
    QTreeWidgetItem* models_item_ = nullptr;
    QTreeWidgetItem* textures_item_ = nullptr;
    QTreeWidgetItem* shaders_item_ = nullptr;
    QTreeWidgetItem* materials_item_ = nullptr;
};
