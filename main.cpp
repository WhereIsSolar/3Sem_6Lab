#include <iostream>
#include <pqxx/pqxx>
#include <string>
#include <vector>
#include <iomanip>

class PharmaDB {
private:
    pqxx::connection* conn;
    
    void printResult(const pqxx::result& res) {
        if (res.empty()) {
            std::cout << "Нет данных" << std::endl;
            return;
        }
        
        for (size_t i = 0; i < res.size(); ++i) {
            const auto& row = res[i];
            std::cout << "\n--- Запись " << (i + 1) << " ---" << std::endl;
            
            for (size_t j = 0; j < row.size(); ++j) {
                std::string col_name = res.column_name(j);
                std::string value = row[j].is_null() ? "Нет данных" : row[j].c_str();
                
                std::string display_name = col_name;
                if (col_name == "name") display_name = "Название";
                else if (col_name == "active_substance") display_name = "Действующее вещество";
                else if (col_name == "dosage_form") display_name = "Форма выпуска";
                else if (col_name == "price") display_name = "Цена";
                else if (col_name == "prescription_required") display_name = "Требуется рецепт";
                else if (col_name == "stock_quantity") display_name = "Количество на складе";
                else if (col_name == "effect_count") display_name = "Количество побочных эффектов";
                else if (col_name == "interaction_type") display_name = "Тип взаимодействия";
                else if (col_name == "severity") display_name = "Серьезность";
                else if (col_name == "avg_price") display_name = "Средняя цена";
                else if (col_name == "min_price") display_name = "Минимальная цена";
                else if (col_name == "max_price") display_name = "Максимальная цена";
                else if (col_name == "phase") display_name = "Фаза исследования";
                else if (col_name == "status") display_name = "Статус";
                else if (col_name == "success_rate") display_name = "Успешность";
                else if (col_name == "analog") display_name = "Аналог";
                else if (col_name == "economy") display_name = "Экономия";
                
                std::cout << std::left << std::setw(30) << display_name + ":" 
                          << value << std::endl;
            }
        }
        std::cout << std::endl;
    }
    
public:
    PharmaDB(const std::string& conn_str) {
        try {
            conn = new pqxx::connection(conn_str);
            if (conn->is_open()) {
                std::cout << "Подключение к БД установлено" << std::endl;
            }
        } catch (const std::exception &e) {
            std::cerr << "Ошибка подключения: " << e.what() << std::endl;
            exit(1);
        }
    }
    
    ~PharmaDB() {
        if (conn && conn->is_open()) {
            conn->close();
        }
        delete conn;
    }
    
    void execute(const std::string& sql) {
        try {
            pqxx::work txn(*conn);
            txn.exec(sql);
            txn.commit();
        } catch (const std::exception &e) {
            std::cerr << "Ошибка: " << e.what() << std::endl;
        }
    }
    
    pqxx::result query(const std::string& sql) {
        try {
            pqxx::work txn(*conn);
            pqxx::result res = txn.exec(sql);
            txn.commit();
            return res;
        } catch (const std::exception &e) {
            std::cerr << "Ошибка запроса: " << e.what() << std::endl;
            throw;
        }
    }
    
    
    void query1_medicines_by_category(const std::string& category) {
    std::string sql = "SELECT m.name, m.active_substance, m.dosage_form, "
                     "ROUND(m.price::numeric, 2) as price "
                     "FROM medicines m "
                     "JOIN medicine_categories mc ON m.id = mc.medicine_id "
                     "JOIN categories c ON mc.category_id = c.id "
                     "WHERE c.name = '" + category + "' "
                     "ORDER BY m.name";
    std::cout << "\n═══════════════════════════════════════════" << std::endl;
    std::cout << "1. Лекарства категории: " << category << std::endl;
    std::cout << "═══════════════════════════════════════════" << std::endl;
    try {
        auto res = query(sql);
        printResult(res);
    } catch (...) {}
} 


    void query2_prescription_required() {
        std::string sql = "SELECT name, active_substance, dosage_form, "
                         "ROUND(price::numeric, 2) as price, "
                         "CASE WHEN prescription_required THEN 'Да' ELSE 'Нет' END as requires_prescription "
                         "FROM medicines "
                         "WHERE prescription_required = true "
                         "ORDER BY price DESC";
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "2. Лекарства, требующие рецепта" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        try {
            auto res = query(sql);
            printResult(res);
        } catch (...) {}
    }
    
    void query3_side_effects_count() {
        std::string sql = "SELECT m.name, COUNT(se.id) as effect_count "
                         "FROM medicines m "
                         "LEFT JOIN side_effects se ON m.id = se.medicine_id "
                         "GROUP BY m.id, m.name "
                         "HAVING COUNT(se.id) > 0 "
                         "ORDER BY effect_count DESC, m.name";
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "3. Статистика побочных эффектов" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        try {
            auto res = query(sql);
            printResult(res);
        } catch (...) {}
    }
    
    void query4_drug_interactions(const std::string& medicine) {
        std::string sql = "SELECT m2.name as medicine2, di.interaction_type, di.severity, di.description "
                         "FROM drug_interactions di "
                         "JOIN medicines m1 ON di.medicine1_id = m1.id "
                         "JOIN medicines m2 ON di.medicine2_id = m2.id "
                         "WHERE m1.name = '" + medicine + "' "
                         "UNION "
                         "SELECT m1.name as medicine2, di.interaction_type, di.severity, di.description "
                         "FROM drug_interactions di "
                         "JOIN medicines m1 ON di.medicine1_id = m1.id "
                         "JOIN medicines m2 ON di.medicine2_id = m2.id "
                         "WHERE m2.name = '" + medicine + "'";
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "4. Взаимодействия для: " << medicine << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        try {
            auto res = query(sql);
            printResult(res);
        } catch (...) {}
    }
    
    void query5_avg_price_by_form() {
        std::string sql = "SELECT dosage_form, "
                         "ROUND(AVG(price)::numeric, 2) as avg_price, "
                         "ROUND(MIN(price)::numeric, 2) as min_price, "
                         "ROUND(MAX(price)::numeric, 2) as max_price, "
                         "COUNT(*) as medicine_count "
                         "FROM medicines "
                         "WHERE price > 0 AND dosage_form IS NOT NULL "
                         "GROUP BY dosage_form "
                         "HAVING AVG(price) > 0 "
                         "ORDER BY avg_price DESC";
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "5. Статистика цен по формам выпуска" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        try {
            auto res = query(sql);
            printResult(res);
        } catch (...) {}
    }
    
    void query6_clinical_trials_status() {
        std::string sql = "SELECT m.name, ct.phase, "
                         "CASE "
                         "  WHEN ct.end_date IS NOT NULL AND ct.end_date < CURRENT_DATE THEN 'Завершено' "
                         "  WHEN ct.end_date IS NULL THEN 'В процессе' "
                         "  ELSE 'Запланировано' "
                         "END as status, "
                         "ROUND(ct.success_rate::numeric, 1) as success_rate "
                         "FROM clinical_trials ct "
                         "JOIN medicines m ON ct.medicine_id = m.id "
                         "ORDER BY ct.phase, ct.start_date DESC";
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "6. Клинические исследования" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        try {
            auto res = query(sql);
            printResult(res);
        } catch (...) {}
    }
    
    void query7_analogs_cheaper(const std::string& medicine) {
        std::string sql = "SELECT a.name as analog, "
                         "ROUND(a.price::numeric, 2) as analog_price, "
                         "ROUND(m.price::numeric, 2) as original_price, "
                         "ROUND((m.price - a.price)::numeric, 2) as economy "
                         "FROM medicines m "
                         "JOIN analogs an ON m.id = an.original_medicine_id "
                         "JOIN medicines a ON an.analog_medicine_id = a.id "
                         "WHERE m.name = '" + medicine + "' AND a.price < m.price "
                         "ORDER BY economy DESC";
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "7. Более дешевые аналоги для: " << medicine << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        try {
            auto res = query(sql);
            printResult(res);
        } catch (...) {}
    }
    
    void query8_update_stock() {
        std::string sql = "UPDATE medicines "
                         "SET stock_quantity = COALESCE(stock_quantity, 0) + 50 "
                         "WHERE COALESCE(stock_quantity, 0) < 100 "
                         "RETURNING name, stock_quantity";
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "8. Пополнение запасов (+50 единиц)" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        try {
            auto res = query(sql);
            printResult(res);
        } catch (...) {}
    }
    
    void query9_insert_new_medicine() {
        std::string sql = "INSERT INTO medicines (name, active_substance, dosage_form, "
                         "price, prescription_required) VALUES "
                         "('Новый препарат', 'Тестовое вещество', 'таблетки', "
                         "250.00, false) "
                         "RETURNING id, name, "
                         "ROUND(price::numeric, 2) as price";
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "9. Добавление нового лекарства" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        try {
            auto res = query(sql);
            printResult(res);
        } catch (...) {}
    }
    
    void query10_expensive_no_prescription() {
        std::string sql = "SELECT name, active_substance, "
                         "ROUND(price::numeric, 2) as price "
                         "FROM medicines "
                         "WHERE prescription_required = false AND price > "
                         "(SELECT AVG(price) FROM medicines WHERE prescription_required = false) "
                         "ORDER BY price DESC";
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "10. Дорогие лекарства без рецепта" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        try {
            auto res = query(sql);
            printResult(res);
        } catch (...) {}
    }
    

void query11_most_interactions() {
    std::string sql = R"(
        SELECT 
            m.name as "Название",
            COUNT(di.id) as "Всего взаимодействий",
            SUM(CASE WHEN di.severity = 'опасное' THEN 1 ELSE 0 END) as "Опасных",
            SUM(CASE WHEN di.severity = 'умеренное' THEN 1 ELSE 0 END) as "Умеренных",
            SUM(CASE WHEN di.severity = 'легкое' THEN 1 ELSE 0 END) as "Легких",
            STRING_AGG(DISTINCT m2.name, ', ') as "Взаимодействует с"
        FROM medicines m
        LEFT JOIN drug_interactions di ON (m.id = di.medicine1_id OR m.id = di.medicine2_id)
        LEFT JOIN medicines m2 ON (
            (di.medicine1_id = m.id AND di.medicine2_id = m2.id) OR 
            (di.medicine2_id = m.id AND di.medicine1_id = m2.id)
        )
        GROUP BY m.id, m.name
        HAVING COUNT(di.id) > 0
        ORDER BY "Всего взаимодействий" DESC, "Опасных" DESC
    )";
    
    std::cout << "\n═══════════════════════════════════════════" << std::endl;
    std::cout << "11. Лекарства с наибольшим числом взаимодействий" << std::endl;
    std::cout << "═══════════════════════════════════════════" << std::endl;
    try {
        auto res = query(sql);
        printResult(res);
    } catch (...) {}
}

void query12_full_profile(const std::string& medicine) {
    std::string sql1 = R"(
        SELECT 
            m.name as "Название",
            m.active_substance as "Действующее вещество",
            m.dosage_form as "Форма выпуска",
            ROUND(m.price::numeric, 2) as "Цена",
            CASE WHEN m.prescription_required THEN 'Да' ELSE 'Нет' END as "Требует рецепта",
            m.stock_quantity as "На складе"
        FROM medicines m 
        WHERE m.name = ')" + medicine + R"('
    )";
    
    std::string sql2 = R"(
        SELECT 
            description as "Побочный эффект",
            severity as "Серьезность"
        FROM side_effects se
        JOIN medicines m ON se.medicine_id = m.id
        WHERE m.name = ')" + medicine + R"('
        ORDER BY severity
    )";
    
    std::string sql3 = R"(
        SELECT 
            m2.name as "Взаимодействует с",
            di.interaction_type as "Тип взаимодействия",
            di.severity as "Опасность",
            di.description as "Описание"
        FROM drug_interactions di
        JOIN medicines m1 ON di.medicine1_id = m1.id
        JOIN medicines m2 ON di.medicine2_id = m2.id
        WHERE m1.name = ')" + medicine + R"('
        UNION
        SELECT 
            m1.name as "Взаимодействует с",
            di.interaction_type as "Тип взаимодействия",
            di.severity as "Опасность",
            di.description as "Описание"
        FROM drug_interactions di
        JOIN medicines m1 ON di.medicine1_id = m1.id
        JOIN medicines m2 ON di.medicine2_id = m2.id
        WHERE m2.name = ')" + medicine + R"('
        ORDER BY "Опасность" DESC
    )";
    
    std::string sql4 = R"(
        SELECT 
            a.name as "Аналог",
            ROUND(a.price::numeric, 2) as "Цена аналога",
            ROUND(an.similarity_percentage::numeric, 1) as "Сходство %",
            CASE 
                WHEN a.price < m.price THEN 'Дешевле на ' || ROUND((m.price - a.price)::numeric, 2) || ' руб.'
                WHEN a.price > m.price THEN 'Дороже на ' || ROUND((a.price - m.price)::numeric, 2) || ' руб.'
                ELSE 'Такая же цена'
            END as "Разница в цене"
        FROM analogs an
        JOIN medicines m ON an.original_medicine_id = m.id
        JOIN medicines a ON an.analog_medicine_id = a.id
        WHERE m.name = ')" + medicine + R"('
        ORDER BY an.similarity_percentage DESC
    )";
    
    std::string sql5 = R"(
        SELECT 
            ct.phase as "Фаза",
            ct.start_date as "Начало",
            ct.end_date as "Окончание",
            ROUND(ct.success_rate::numeric, 1) as "Успешность %",
            CASE 
                WHEN ct.end_date IS NULL THEN 'В процессе'
                WHEN ct.end_date < CURRENT_DATE THEN 'Завершено'
                ELSE 'Запланировано'
            END as "Статус"
        FROM clinical_trials ct
        JOIN medicines m ON ct.medicine_id = m.id
        WHERE m.name = ')" + medicine + R"('
        ORDER BY ct.phase
    )";
    
    std::cout << "\n═══════════════════════════════════════════" << std::endl;
    std::cout << "12. Полный профиль: " << medicine << std::endl;
    std::cout << "═══════════════════════════════════════════" << std::endl;
    
    try {
        std::cout << "\n=== ОСНОВНАЯ ИНФОРМАЦИЯ ===" << std::endl;
        auto res1 = query(sql1);
        printResult(res1);
        
        std::cout << "\n=== ПОБОЧНЫЕ ЭФФЕКТЫ ===" << std::endl;
        auto res2 = query(sql2);
        if (res2.empty()) {
            std::cout << "Нет данных о побочных эффектах" << std::endl;
        } else {
            printResult(res2);
        }
        
        std::cout << "\n=== ВЗАИМОДЕЙСТВИЯ С ДРУГИМИ ЛЕКАРСТВАМИ ===" << std::endl;
        auto res3 = query(sql3);
        if (res3.empty()) {
            std::cout << "Нет данных о взаимодействиях" << std::endl;
        } else {
            printResult(res3);
        }
        
        std::cout << "\n=== АНАЛОГИ ===" << std::endl;
        auto res4 = query(sql4);
        if (res4.empty()) {
            std::cout << "Аналоги не найдены" << std::endl;
        } else {
            printResult(res4);
        }
        
        std::cout << "\n=== КЛИНИЧЕСКИЕ ИССЛЕДОВАНИЯ ===" << std::endl;
        auto res5 = query(sql5);
        if (res5.empty()) {
            std::cout << "Нет данных о клинических исследованиях" << std::endl;
        } else {
            printResult(res5);
        }
        
    } catch (...) {}
}
    // SQL-ИНЪЕКЦИИ
    
    void injection1_vulnerable_login() {
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "SQL-инъекция 1: Уязвимый логин" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        std::cout << "Уязвимый запрос:" << std::endl;
        std::cout << "SELECT * FROM users WHERE username = '$username' AND password = '$password'" << std::endl;
        std::cout << "\nАтака:" << std::endl;
        std::cout << "Username: admin' --" << std::endl;
        std::cout << "Password: любой" << std::endl;
        std::cout << "\nИтоговый SQL:" << std::endl;
        std::cout << "SELECT * FROM users WHERE username = 'admin' --' AND password = 'любой'" << std::endl;
        std::cout << "\nРезультат: получаем доступ как admin" << std::endl;
    }
    
    void injection2_vulnerable_search() {
        std::string search = "' OR '1'='1";
        std::string sql = "SELECT * FROM medicines WHERE name LIKE '%" + search + "%'";
        
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "SQL-инъекция 2: Уязвимый поиск" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        std::cout << "Пользовательский ввод: " << search << std::endl;
        std::cout << "\nСформированный SQL:" << std::endl;
        std::cout << sql << std::endl;
        std::cout << "\nРезультат: покажет ВСЕ лекарства" << std::endl;
        
        try {
            auto res = query(sql);
            std::cout << "\nНайдено записей: " << res.size() << std::endl;
        } catch (...) {}
    }
    
    void injection3_union_attack() {
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "SQL-инъекция 3: UNION-атака" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        std::cout << "Уязвимый запрос:" << std::endl;
        std::cout << "SELECT name, price FROM medicines WHERE id = $id" << std::endl;
        std::cout << "\nАтака (id):" << std::endl;
        std::cout << "1 UNION SELECT username, password FROM users" << std::endl;
        std::cout << "\nИтоговый SQL:" << std::endl;
        std::cout << "SELECT name, price FROM medicines WHERE id = 1 UNION SELECT username, password FROM users" << std::endl;
        std::cout << "\nРезультат: получаем пароли пользователей" << std::endl;
    }
    
    void injection4_error_based() {
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "SQL-инъекция 4: Error-based" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        std::cout << "Уязвимый запрос:" << std::endl;
        std::cout << "SELECT * FROM medicines WHERE id = $id" << std::endl;
        std::cout << "\nАтака (id):" << std::endl;
        std::cout << "1 AND 1=CAST((SELECT version()) AS INT)" << std::endl;
        std::cout << "\nРезультат: ошибка БД покажет версию PostgreSQL" << std::endl;
    }
    
    void injection5_time_based() {
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "SQL-инъекция 5: Time-based (Blind)" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        std::cout << "Уязвимый запрос:" << std::endl;
        std::cout << "SELECT * FROM users WHERE username = '$username'" << std::endl;
        std::cout << "\nАтака (username):" << std::endl;
        std::cout << "admin' AND (SELECT pg_sleep(5))--" << std::endl;
        std::cout << "\nРезультат: если ответ задерживается 5 сек, значит пользователь 'admin' существует" << std::endl;
    }
    
    // ЗАЩИЩЕННЫЕ МЕТОДЫ
    
    void safe_search(const std::string& search_term) {
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "Безопасный поиск" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        
        try {
            pqxx::work txn(*conn);
            std::string sql = "SELECT name, active_substance, "
                             "ROUND(price::numeric, 2) as price "
                             "FROM medicines WHERE name LIKE $1";
            pqxx::params params;
            params.append("%" + search_term + "%");
            pqxx::result res = txn.exec(sql, params);
            txn.commit();
            
            std::cout << "Поиск: " << search_term << std::endl;
            std::cout << "Найдено записей: " << res.size() << std::endl;
            printResult(res);
            
        } catch (const std::exception &e) {
            std::cerr << "Ошибка: " << e.what() << std::endl;
        }
    }
    
    void safe_insert() {
        std::string name, substance;
        double price;
        
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "Безопасное добавление лекарства" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        
        std::cout << "Название: ";
        std::cin.ignore();
        std::getline(std::cin, name);
        std::cout << "Действующее вещество: ";
        std::getline(std::cin, substance);
        std::cout << "Цена: ";
        std::cin >> price;
        
        try {
            pqxx::work txn(*conn);
            std::string sql = "INSERT INTO medicines (name, active_substance, price) "
                             "VALUES ($1, $2, $3) RETURNING id, name";
            pqxx::params params;
            params.append(name);
            params.append(substance);
            params.append(price);
            pqxx::result res = txn.exec(sql, params);
            txn.commit();
            
            std::cout << "\n Лекарство успешно добавлено!" << std::endl;
            std::cout << "ID: " << res[0]["id"].c_str() << std::endl;
            std::cout << "Название: " << res[0]["name"].c_str() << std::endl;
            
        } catch (const std::exception &e) {
            std::cerr << "Ошибка: " << e.what() << std::endl;
        }
    }
    
    // ИНИЦИАЛИЗАЦИЯ БАЗЫ
    
    void init_database() {
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "Инициализация базы данных" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        
        try {
            pqxx::work txn(*conn);
            
            // Таблицы
            std::vector<std::string> tables = {
                "CREATE TABLE IF NOT EXISTS medicines ("
                "id SERIAL PRIMARY KEY,"
                "name VARCHAR(200) NOT NULL,"
                "active_substance VARCHAR(200) NOT NULL,"
                "dosage_form VARCHAR(50),"
                "dosage VARCHAR(100),"
                "manufacturer VARCHAR(200),"
                "country VARCHAR(100),"
                "registration_date DATE,"
                "price DECIMAL(10, 2),"
                "prescription_required BOOLEAN DEFAULT true,"
                "stock_quantity INTEGER DEFAULT 0)",
                
                "CREATE TABLE IF NOT EXISTS categories ("
                "id SERIAL PRIMARY KEY,"
                "name VARCHAR(100) NOT NULL UNIQUE,"
                "description TEXT)",
                
                "CREATE TABLE IF NOT EXISTS medicine_categories ("
                "medicine_id INTEGER,"
                "category_id INTEGER,"
                "PRIMARY KEY (medicine_id, category_id))",
                
                "CREATE TABLE IF NOT EXISTS side_effects ("
                "id SERIAL PRIMARY KEY,"
                "medicine_id INTEGER NOT NULL,"
                "description TEXT NOT NULL,"
                "frequency VARCHAR(50),"
                "severity VARCHAR(20))",
                
                "CREATE TABLE IF NOT EXISTS drug_interactions ("
                "id SERIAL PRIMARY KEY,"
                "medicine1_id INTEGER NOT NULL,"
                "medicine2_id INTEGER NOT NULL,"
                "interaction_type VARCHAR(100) NOT NULL,"
                "description TEXT NOT NULL,"
                "severity VARCHAR(20),"
                "UNIQUE(medicine1_id, medicine2_id))",
                
                "CREATE TABLE IF NOT EXISTS clinical_trials ("
                "id SERIAL PRIMARY KEY,"
                "medicine_id INTEGER NOT NULL,"
                "phase INTEGER,"
                "start_date DATE NOT NULL,"
                "end_date DATE,"
                "participants_count INTEGER,"
                "success_rate DECIMAL(5, 2),"
                "results_summary TEXT)",
                
                "CREATE TABLE IF NOT EXISTS analogs ("
                "id SERIAL PRIMARY KEY,"
                "original_medicine_id INTEGER NOT NULL,"
                "analog_medicine_id INTEGER NOT NULL,"
                "similarity_percentage DECIMAL(5, 2),"
                "notes TEXT,"
                "UNIQUE(original_medicine_id, analog_medicine_id))"
            };
            
            // Создаем таблицы
            for (const auto& sql : tables) {
                txn.exec(sql);
            }
            
            // Внешние ключи
            std::vector<std::string> fkeys = {
                "ALTER TABLE medicine_categories ADD CONSTRAINT fk_med_cat_med "
                "FOREIGN KEY (medicine_id) REFERENCES medicines(id) ON DELETE CASCADE",
                
                "ALTER TABLE medicine_categories ADD CONSTRAINT fk_med_cat_cat "
                "FOREIGN KEY (category_id) REFERENCES categories(id) ON DELETE CASCADE",
                
                "ALTER TABLE side_effects ADD CONSTRAINT fk_side_effects_med "
                "FOREIGN KEY (medicine_id) REFERENCES medicines(id) ON DELETE CASCADE",
                
                "ALTER TABLE drug_interactions ADD CONSTRAINT fk_interactions_med1 "
                "FOREIGN KEY (medicine1_id) REFERENCES medicines(id) ON DELETE CASCADE",
                
                "ALTER TABLE drug_interactions ADD CONSTRAINT fk_interactions_med2 "
                "FOREIGN KEY (medicine2_id) REFERENCES medicines(id) ON DELETE CASCADE",
                
                "ALTER TABLE clinical_trials ADD CONSTRAINT fk_trials_med "
                "FOREIGN KEY (medicine_id) REFERENCES medicines(id) ON DELETE CASCADE",
                
                "ALTER TABLE analogs ADD CONSTRAINT fk_analogs_original "
                "FOREIGN KEY (original_medicine_id) REFERENCES medicines(id) ON DELETE CASCADE",
                
                "ALTER TABLE analogs ADD CONSTRAINT fk_analogs_analog "
                "FOREIGN KEY (analog_medicine_id) REFERENCES medicines(id) ON DELETE CASCADE"
            };
            
            for (const auto& sql : fkeys) {
                try {
                    txn.exec(sql);
                } catch (...) {
                    // Пропускаем если уже существуют
                }
            }
            
            txn.commit();
            std::cout << " Таблицы созданы успешно!" << std::endl;
            
        } catch (const std::exception &e) {
            std::cerr << "Ошибка: " << e.what() << std::endl;
        }
    }
    
    void seed_data() {
    std::cout << "\n═══════════════════════════════════════════" << std::endl;
    std::cout << "Заполнение тестовыми данными" << std::endl;
    std::cout << "═══════════════════════════════════════════" << std::endl;
    
    try {
        pqxx::work cleanup(*conn);
        cleanup.exec("TRUNCATE analogs, clinical_trials, drug_interactions, "
                    "side_effects, medicine_categories, categories, medicines CASCADE");
        cleanup.commit();
        
        pqxx::work txn(*conn);
        
        // 1. Временно отключаем проверку внешних ключей
        txn.exec("SET CONSTRAINTS ALL DEFERRED");
        
        // 2. Сначала добавляем лекарства
        txn.exec(R"(
            INSERT INTO medicines (id, name, active_substance, dosage_form, price, prescription_required) VALUES 
            (1, 'Аспирин', 'ацетилсалициловая кислота', 'таблетки', 150.00, false),
            (2, 'Ибупрофен', 'ибупрофен', 'таблетки', 120.00, false),
            (3, 'Парацетамол', 'парацетамол', 'таблетки', 80.00, false),
            (4, 'Амоксиклав', 'амоксициллин', 'таблетки', 350.00, true),
            (5, 'Ношпа', 'дротаверин', 'таблетки', 200.00, false),
            (6, 'Эналаприл', 'эналаприл', 'таблетки', 180.00, true),
            (7, 'Метформин', 'метформин', 'таблетки', 160.00, true),
            (8, 'Аторвастатин', 'аторвастатин', 'таблетки', 220.00, true)
        )");
        std::cout << " Лекарства добавлены" << std::endl;
        
        // 3. Добавляем категории
        txn.exec(R"(
            INSERT INTO categories (id, name, description) VALUES 
            (1, 'Обезболивающие', 'Для снятия боли'),
            (2, 'Антибиотики', 'Против инфекций'),
            (3, 'Сердечно-сосудистые', 'Для сердца'),
            (4, 'Противодиабетические', 'От диабета'),
            (5, 'Жаропонижающие', 'Снижают температуру')
        )");
        std::cout << " Категории добавлены" << std::endl;
        
        // 4. Связи лекарств и категорий (используем явные ID)
        txn.exec(R"(
            INSERT INTO medicine_categories (medicine_id, category_id) VALUES 
            (1, 1), (1, 5),
            (2, 1), (2, 5),
            (3, 1), (3, 5),
            (4, 2),
            (6, 3),
            (7, 4),
            (8, 3)
        )");
        std::cout << " Связи добавлены" << std::endl;
        
        // 5. Побочные эффекты
        txn.exec(R"(
            INSERT INTO side_effects (medicine_id, description, severity) VALUES 
            (1, 'Тошнота, изжога', 'легкая'),
            (4, 'Аллергия, диарея', 'умеренная'),
            (6, 'Головокружение, кашель', 'легкая'),
            (8, 'Боль в мышцах', 'умеренная')
        )");
        std::cout << " Побочные эффекты добавлены" << std::endl;
        
        // 6. Взаимодействия лекарств
        txn.exec(R"(
            INSERT INTO drug_interactions (medicine1_id, medicine2_id, interaction_type, description, severity) VALUES 
            (1, 2, 'Усиление эффекта', 'Риск кровотечения', 'умеренное'),
            (6, 8, 'Опасное сочетание', 'Риск для почек', 'опасное')
        )");
        std::cout << " Взаимодействия добавлены" << std::endl;
        
        // 7. Аналоги
        txn.exec(R"(
            INSERT INTO analogs (original_medicine_id, analog_medicine_id, similarity_percentage) VALUES 
            (1, 2, 70),
            (1, 3, 65),
            (6, 8, 60)
        )");
        std::cout << " Аналоги добавлены" << std::endl;
        
        // 8. Клинические исследования
        txn.exec(R"(
            INSERT INTO clinical_trials (medicine_id, phase, start_date, success_rate) VALUES 
            (1, 4, '2020-01-01', 92.5),
            (8, 3, '2022-06-01', 87.3)
        )");
        std::cout << " Клинические исследования добавлены" << std::endl;
        
        // 9. Фиксируем транзакцию
        txn.commit();
        
        // 10. Сбрасываем последовательности
        pqxx::work reset_seq(*conn);
        reset_seq.exec("SELECT setval('medicines_id_seq', COALESCE((SELECT MAX(id) FROM medicines), 0) + 1, false)");
        reset_seq.exec("SELECT setval('categories_id_seq', COALESCE((SELECT MAX(id) FROM categories), 0) + 1, false)");
        reset_seq.commit();
        
        std::cout << "\n Все тестовые данные успешно добавлены!" << std::endl;
        
        // Проверка
        pqxx::work check(*conn);
        pqxx::result res = check.exec("SELECT COUNT(*) as cnt FROM medicines");
        std::cout << "Всего лекарств в БД: " << res[0]["cnt"].c_str() << std::endl;
        check.commit();
        
    } catch (const std::exception &e) {
        std::cerr << " Ошибка: " << e.what() << std::endl;
    }
}
};



void execute_10_queries(PharmaDB& db) {
    std::cout << "\n═══════════════════════════════════════════" << std::endl;
    std::cout << "   ВЫПОЛНЕНИЕ 10 ОСНОВНЫХ ЗАПРОСОВ" << std::endl;
    std::cout << "═══════════════════════════════════════════" << std::endl;
    
    int query_choice;
    do {
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "Выберите запрос (1-10) или 0 для выхода:" << std::endl;
        std::cout << "1. Лекарства по категории" << std::endl;
        std::cout << "2. Лекарства, требующие рецепта" << std::endl;
        std::cout << "3. Статистика побочных эффектов" << std::endl;
        std::cout << "4. Взаимодействия лекарств" << std::endl;
        std::cout << "5. Статистика цен по формам выпуска" << std::endl;
        std::cout << "6. Клинические исследования" << std::endl;
        std::cout << "7. Дешевые аналоги" << std::endl;
        std::cout << "8. Пополнение запасов" << std::endl;
        std::cout << "9. Добавление нового лекарства" << std::endl;
        std::cout << "10. Дорогие лекарства без рецепта" << std::endl;
        std::cout << "11. Лекарства с самым большим количеством взаимодействий" << std::endl;
        std::cout << "12. Полный профиль лекарства" << std::endl;
        std::cout << "0. Выход в главное меню" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        std::cout << "Выбор: ";
        std::cin >> query_choice;
        
        switch(query_choice) {
            case 1: {
                std::string category;
                std::cout << "\nВведите название категории (например: Обезболивающие): ";
                std::cin.ignore();
                std::getline(std::cin, category);
                db.query1_medicines_by_category(category);
                break;
            }
            case 2:
                db.query2_prescription_required();
                break;
            case 3:
                db.query3_side_effects_count();
                break;
            case 4: {
                std::string medicine;
                std::cout << "\nВведите название лекарства (например: Аспирин): ";
                std::cin.ignore();
                std::getline(std::cin, medicine);
                db.query4_drug_interactions(medicine);
                break;
            }
            case 5:
                db.query5_avg_price_by_form();
                break;
            case 6:
                db.query6_clinical_trials_status();
                break;
            case 7: {
                std::string medicine;
                std::cout << "\nВведите название лекарства (например: Аспирин): ";
                std::cin.ignore();
                std::getline(std::cin, medicine);
                db.query7_analogs_cheaper(medicine);
                break;
            }
            case 8:
                db.query8_update_stock();
                break;
            case 9:
                db.query9_insert_new_medicine();
                break;
            case 10:
                db.query10_expensive_no_prescription();
                break;
            case 11:
                db.query11_most_interactions();
                break;
            case 12: {
                std::string medicine;
                std::cout << "\nВведите название лекарства для полного профиля: ";
                std::cin.ignore();
                std::getline(std::cin, medicine);
                db.query12_full_profile(medicine);
                break;
            }
            
            case 0:
                std::cout << "Возврат в главное меню..." << std::endl;
                break;
            default:
                std::cout << "Неверный выбор!" << std::endl;
        }
    } while (query_choice != 0);
}

void execute_sql_injections(PharmaDB& db) {
    std::cout << "\n═══════════════════════════════════════════" << std::endl;
    std::cout << "      ДЕМОНСТРАЦИЯ SQL-ИНЪЕКЦИЙ" << std::endl;
    std::cout << "═══════════════════════════════════════════" << std::endl;
    
    int injection_choice;
    do {
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "Выберите тип инъекции (1-5) или 0 для выхода:" << std::endl;
        std::cout << "1. Уязвимый логин" << std::endl;
        std::cout << "2. Уязвимый поиск" << std::endl;
        std::cout << "3. UNION-атака" << std::endl;
        std::cout << "4. Error-based атака" << std::endl;
        std::cout << "5. Time-based атака (Blind)" << std::endl;
        std::cout << "0. Выход в главное меню" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        std::cout << "Выбор: ";
        std::cin >> injection_choice;
        
        switch(injection_choice) {
            case 1:
                db.injection1_vulnerable_login();
                break;
            case 2:
                db.injection2_vulnerable_search();
                break;
            case 3:
                db.injection3_union_attack();
                break;
            case 4:
                db.injection4_error_based();
                break;
            case 5:
                db.injection5_time_based();
                break;
            case 0:
                std::cout << "Возврат в главное меню..." << std::endl;
                break;
            default:
                std::cout << "Неверный выбор!" << std::endl;
        }
    } while (injection_choice != 0);
}

int main() {
    std::string conn_str = "host=localhost port=5432 dbname=farmacologia user=postgres password=ваш_пароль";
    
    PharmaDB db(conn_str);
    
    int choice;
    do {
        std::cout << "\n═══════════════════════════════════════════" << std::endl;
        std::cout << "      ФАРМАКОЛОГИЧЕСКАЯ БАЗА ДАННЫХ" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        std::cout << "1. Инициализировать таблицы" << std::endl;
        std::cout << "2. Заполнить тестовыми данными" << std::endl;
        std::cout << "3. Выполнить 10 основных запросов" << std::endl;
        std::cout << "4. Демонстрация SQL-инъекций" << std::endl;
        std::cout << "5. Безопасный поиск лекарства" << std::endl;
        std::cout << "6. Безопасное добавление лекарства" << std::endl;
        std::cout << "0. Выход" << std::endl;
        std::cout << "═══════════════════════════════════════════" << std::endl;
        std::cout << "Выбор: ";
        std::cin >> choice;
        
        switch(choice) {
            case 1:
                db.init_database();
                break;
            case 2:
                db.seed_data();
                break;
            case 3:
                execute_10_queries(db);
                break;
            case 4:
                execute_sql_injections(db);
                break;
            case 5: {
                std::string search;
                std::cout << "Введите название для поиска: ";
                std::cin.ignore();
                std::getline(std::cin, search);
                db.safe_search(search);
                break;
            }
            case 6:
                db.safe_insert();
                break;
            case 0:
                std::cout << "Выход из программы..." << std::endl;
                break;
            default:
                std::cout << "Неверный выбор!" << std::endl;
        }
    } while (choice != 0);
    
    return 0;
}

