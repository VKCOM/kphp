#include <array>
#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "runtime/vkext.h"

using casing_table = std::array<std::string, 6>;

class FlexTest : public ::testing::Test {
protected:
  struct {
    std::vector<casing_table> male_names = {
      {"Алексей", "Алексея", "Алексею", "Алексея", "Алексеем", "Алексее"},
      {"Павел", "Павла", "Павлу", "Павла", "Павлом", "Павле"},
      {"Михаил", "Михаила", "Михаилу", "Михаила", "Михаилом", "Михаиле"},
      {"Никита", "Никиты", "Никите", "Никиту", "Никитой", "Никите"},
      {"Илья", "Ильи", "Илье", "Илью", "Ильёй", "Илье"},
      {"Ромео", "Ромео", "Ромео", "Ромео", "Ромео", "Ромео"},
      {"Рене", "Рене", "Рене", "Рене", "Рене", "Рене"},
    };
    std::vector<casing_table> male_surnames = {
      {"Иванов", "Иванова", "Иванову", "Иванова", "Ивановым", "Иванове"},
      {"Пушкин", "Пушкина", "Пушкину", "Пушкина", "Пушкиным", "Пушкине"},
      {"Бедный", "Бедного", "Бедному", "Бедного", "Бедным", "Бедном"},
      {"Агумая", "Агумаи", "Агумае", "Агумаю", "Агумаей", "Агумае"},
    };
    std::vector<casing_table> female_names = {
      {"Марина", "Марины", "Марине", "Марину", "Мариной", "Марине"},       {"Мария", "Марии", "Марии", "Марию", "Марией", "Марии"},
      {"Наталья", "Натальи", "Наталье", "Наталью", "Натальей", "Наталье"}, {"Любовь", "Любови", "Любови", "Любовь", "Любовью", "Любови"},
      {"Жизель", "Жизели", "Жизели", "Жизель", "Жизелью", "Жизели"},       {"Ирен", "Ирен", "Ирен", "Ирен", "Ирен", "Ирен"},
    };
    std::vector<casing_table> female_surnames = {
      {"Иванова", "Ивановой", "Ивановой", "Иванову", "Ивановой", "Ивановой"},
      {"Пушкина", "Пушкиной", "Пушкиной", "Пушкину", "Пушкиной", "Пушкиной"},
      {"Бедная", "Бедной", "Бедной", "Бедную", "Бедной", "Бедной"},
      {"Агумая", "Агумой", "Агумой", "Агумую", "Агумой", "Агумой"},
    };
  } basic;

  struct {
    std::vector<casing_table> surnames = {
      {"Ялыця", "Ялыця", "Ялыця", "Ялыця", "Ялыця", "Ялыця"},
      {"Дюма", "Дюма", "Дюма", "Дюма", "Дюма", "Дюма"},
      {"Гюго", "Гюго", "Гюго", "Гюго", "Гюго", "Гюго"},
      {"Коваленко", "Коваленко", "Коваленко", "Коваленко", "Коваленко", "Коваленко"},
      {"Живаго", "Живаго", "Живаго", "Живаго", "Живаго", "Живаго"},
      {"Галуа", "Галуа", "Галуа", "Галуа", "Галуа", "Галуа"},
      {"Моравиа", "Моравиа", "Моравиа", "Моравиа", "Моравиа", "Моравиа"},
      {"Золя", "Золя", "Золя", "Золя", "Золя", "Золя"},
      {"Шмутцих", "Шмутцих", "Шмутцих", "Шмутцих", "Шмутцих", "Шмутцих"},
      {"Веселых", "Веселых", "Веселых", "Веселых", "Веселых", "Веселых"},
    };
  } non_casing;

  struct {
    std::vector<casing_table> surnames = {
      {"Кожемяка", "Кожемяки", "Кожемяке", "Кожемяку", "Кожемякой", "Кожемяке"},
      {"Байда", "Байды", "Байде", "Байду", "Байдой", "Байде"},
      {"Вернигора", "Вернигоры", "Вернигоре", "Вернигору", "Вернигорой", "Вернигоре"},
      {"Данелия", "Данелии", "Данелии", "Данелию", "Данелией", "Данелии"},
    };
  } gender_independent;

  struct {
    std::vector<casing_table> male_surnames = {
      {"Круг", "Круга", "Кругу", "Круга", "Кругом", "Круге"},
      {"Гайдай", "Гайдая", "Гайдаю", "Гайдая", "Гайдаем", "Гайдае"},
      {"Зюзь", "Зюзя", "Зюзю", "Зюзя", "Зюзем", "Зюзе"},
      {"Гришковец", "Гришковца", "Гришковцу", "Гришковца", "Гришковцом", "Гришковце"},
    };
    std::vector<casing_table> female_surnames = {
      {"Круг", "Круг", "Круг", "Круг", "Круг", "Круг"},
      {"Гайдай", "Гайдай", "Гайдай", "Гайдай", "Гайдай", "Гайдай"},
      {"Зюзь", "Зюзь", "Зюзь", "Зюзь", "Зюзь", "Зюзь"},
      {"Гришковец", "Гришковец", "Гришковец", "Гришковец", "Гришковец", "Гришковец"},
    };
  } gender_specific;

  std::vector<casing_table> get_all_surnames(bool is_female) const {
    std::vector<casing_table> res;
    res.insert(res.cend(), non_casing.surnames.begin(), non_casing.surnames.end());
    res.insert(res.cend(), gender_independent.surnames.begin(), gender_independent.surnames.end());
    if (is_female) {
      res.insert(res.cend(), basic.female_surnames.begin(), basic.female_surnames.end());
      res.insert(res.cend(), gender_specific.female_surnames.begin(), gender_specific.female_surnames.end());
    } else {
      res.insert(res.cend(), basic.male_surnames.begin(), basic.male_surnames.end());
      res.insert(res.cend(), gender_specific.male_surnames.begin(), gender_specific.male_surnames.end());
    }
    return res;
  }
};

void test_casing(const casing_table &cased_name, bool is_female, bool is_surname) {
  static const char *cases[] = {"Gen", "Dat", "Acc", "Ins", "Abl"};
  string name{cased_name.front().c_str()};
  name = f$vk_utf8_to_win(name);
  int i = 1;
  for (const auto *case_ : cases) {
    string cased = f$vk_flex(name, string{case_}, is_female, is_surname ? string{"surnames"} : string{"names"});
    cased = f$vk_win_to_utf8(cased);
    ASSERT_EQ(std::string{cased.c_str()}, cased_name.at(i++));
  }
}

void test_double_names(const std::vector<casing_table> &name_tables, bool is_female, bool is_surname) {
  for (const auto &name_table_1 : name_tables) {
    for (const auto &name_table_2 : name_tables) {
      casing_table double_name_casing;
      for (int i = 0; i < double_name_casing.size(); ++i) {
        double_name_casing[i] = name_table_1[i] + "-" + name_table_2[i];
      }
      test_casing(double_name_casing, is_female, is_surname);
    }
  }
}

TEST_F(FlexTest, test_names_basic) {
  for (auto &name : basic.male_names) {
    test_casing(name, false, false);
  }
  for (auto &name : basic.female_names) {
    test_casing(name, true, false);
  }
}

TEST_F(FlexTest, test_names_double) {
  test_double_names(basic.male_names, false, false);
  test_double_names(basic.female_names, true, false);
}

TEST_F(FlexTest, test_surnames_basic) {
  for (auto &surname : basic.male_surnames) {
    test_casing(surname, false, true);
  }
  for (auto &surname : basic.female_surnames) {
    test_casing(surname, true, true);
  }
}

TEST_F(FlexTest, test_surnames_non_casing) {
  for (auto &surname : non_casing.surnames) {
    test_casing(surname, false, true);
    test_casing(surname, true, true);
  }
}

TEST_F(FlexTest, test_surnames_gender_independent) {
  for (auto &surname : gender_independent.surnames) {
    test_casing(surname, false, true);
    test_casing(surname, true, true);
  }
}

TEST_F(FlexTest, test_surnames_gender_specific) {
  for (auto &surname : gender_specific.male_surnames) {
    test_casing(surname, false, true);
  }
  for (auto &surname : gender_specific.female_surnames) {
    test_casing(surname, true, true);
  }
}

TEST_F(FlexTest, test_surnames_double) {
  test_double_names(get_all_surnames(false), false, true);
  test_double_names(get_all_surnames(true), true, true);
}
