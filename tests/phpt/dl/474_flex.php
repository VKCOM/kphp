@ok benchmark
<?php
#ifndef KittenPHP

function vk_flex($name, $case, $sex, $type, $lang_id = 0) {
  if ($sex != 1) $sex = 0;
  if (!$case || $case == 'Nom') return $name;

  static $flex_langs = array();
  if (!isset($flex_langs[$lang_id])) {
    $res = array();
    if (in_array($lang_id, array(0, 11, 19, 52, 777, 888, 999))) {
$res['flexible_symbols'] = "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЬЭЮЯабвгдеёжзийклмнопрстуфхцчшщьэюя";

$res['names'] = array(
  array(
    'patterns' => array('*ь'),
    'male' => array('Gen' => 'я', 'Dat' => 'ю', 'Acc' => 'я', 'Ins' => 'ем', 'Abl' => 'е'),
    'female' => array('Gen' => 'и', 'Dat' => 'и', 'Acc' => 'ь', 'Ins' => 'ью', 'Abl' => 'и')
  ),
  array(
    'patterns' => array('Гюзель', 'Гузель', 'Айгуль', 'Айгюль', 'Гюнэль', 'Гюнель', 'Аревик',
'Астхик', 'Шагик', 'Татевик', 'Сатеник', 'Манушак', 'Анушик', 'Хасмик', 'Назик', 'Кайцак', 'Ардак', 'Арпик',
'Гюзяль', 'Жибек', 'Асель', 'Николь', 'Аниель', 'Асмик', 'Ола', 'Эттель', 'Илль', 'Амель', 'Абигаль', '*хаяа',
'Нинель', 'Асаль', 'Назгуль', 'Айсель', 'Изабель'),
    'male' => 'fixed',
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('Даниэл(ь)'),
    'male' => array('Gen' => 'я', 'Dat' => 'ю', 'Acc' => 'я', 'Ins' => 'ем', 'Abl' => 'е'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('Аревик()', 'Астхик()', 'Шагик()', 'Татевик()', 'Сатеник()', 'Манушак()', 'Анушик()', 
'Хасмик()', 'Назик()', 'Кайцак()', 'Ардак()', 'Арпик()', 'Жибек()', 'Асмик()', 'Айчурок()'),
    'male' => array('Gen' => 'а', 'Dat' => 'у', 'Acc' => 'а', 'Ins' => 'ом', 'Abl' => 'е'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*о', '*у', '*ы', '*э', '*ё', '*ю', '*и', '*е'),
    'male' => 'fixed',
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*у(а)', '*и(а)', '*э(а)', '*е(а)', '*ю(а)', '*а(а)', '*о(а)'),
    'male' => 'fixed',
    'female' => 'fixed', 
  ),
  array(
    'patterns' => array('*й'),
    'male' => array('Gen' => 'я', 'Dat' => 'ю', 'Acc' => 'я', 'Ins' => 'ем', 'Abl' => 'е'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*ий'),
    'male' => array('Gen' => 'ия', 'Dat' => 'ию', 'Acc' => 'ия', 'Ins' => 'ием', 'Abl' => 'ии'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*я'),
    'male' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'ю', 'Ins' => 'ей', 'Abl' => 'е'),
    'female' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'ю', 'Ins' => 'ей', 'Abl' => 'е'),
  ),
  array(
    'patterns' => array('*ия'),
    'male' => array('Gen' => 'ии', 'Dat' => 'ие', 'Acc' => 'ию', 'Ins' => 'ией', 'Abl' => 'ие'), // почему m и f разные??
    'female' => array('Gen' => 'ии', 'Dat' => 'ии', 'Acc' => 'ию', 'Ins' => 'ией', 'Abl' => 'ии'),
  ),
  array(
    'patterns' => array('Али(я)', 'Нажи(я)', 'Гали(я)', 'Альфи(я)', 'Балхи(я)', 'Нури(я)', 'Зульфи(я)','Ади(я)'),
    'male' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'ю', 'Ins' => 'ей', 'Abl' => 'е'),
    'female' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'ю', 'Ins' => 'ей', 'Abl' => 'е'),
  ),
  array(
    'patterns' => array('*а'),
    'male' => array('Gen' => 'ы', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'),
    'female' => array('Gen' => 'ы', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'),
  ),
  array(
    'patterns' => array('*к(а)', '*х(а)', '*г(а)'),
    'male' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'),
    'female' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'),
  ),
  array(
    'patterns' => array('*ш(а)', '*ж(а)', '*ч(а)', '*щ(а)'),
    'male' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ей', 'Abl' => 'е'),
    'female' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ей', 'Abl' => 'е'),
  ),
  array(
    'patterns' => array('*ца'),
    'male' => array('Gen' => 'цы', 'Dat' => 'це', 'Acc' => 'цу', 'Ins' => 'цей', 'Abl' => 'це'),
    'female' => array('Gen' => 'цы', 'Dat' => 'це', 'Acc' => 'цу', 'Ins' => 'цей', 'Abl' => 'це'),
  ),
  array(
    'patterns' => array('Миляуш(а)'),
    'male' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'),
    'female' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'),
  ),
  array(
    'patterns' => array('*'),
    'male' => array('Gen' => 'а', 'Dat' => 'у', 'Acc' => 'а', 'Ins' => 'ом', 'Abl' => 'е'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Пётр'),
    'male' => array('Gen' => 'Петра', 'Dat' => 'Петру', 'Acc' => 'Петра', 'Ins' => 'Петром', 'Abl' => 'Петре'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Павел'),
    'male' => array('Gen' => 'Павла', 'Dat' => 'Павлу', 'Acc' => 'Павла', 'Ins' => 'Павлом', 'Abl' => 'Павле'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Лев'),
    'male' => array('Gen' => 'Льва', 'Dat' => 'Льву', 'Acc' => 'Льва', 'Ins' => 'Львом', 'Abl' => 'Льве'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('*а(ёк)', '*о(ёк)', '*у(ёк)', '*е(ёк)', '*и(ёк)'),
    'male' => array('Gen' => 'йка', 'Dat' => 'йку', 'Acc' => 'йка', 'Ins' => 'йком', 'Abl' => 'йке'),
    'female' => array('Gen' => 'йка', 'Dat' => 'йку', 'Acc' => 'йка', 'Ins' => 'йком', 'Abl' => 'йке'),
  ),
  array(
    'patterns' => array('*ёк'),
    'male' => array('Gen' => 'ька', 'Dat' => 'ьку', 'Acc' => 'ька', 'Ins' => 'ьком', 'Abl' => 'ьке'),
    'female' => array('Gen' => 'ька', 'Dat' => 'ьку', 'Acc' => 'ька', 'Ins' => 'ьком', 'Abl' => 'ьке'),
  ),
  array(
    'patterns' => array('*ок'),
    'male' => array('Gen' => 'ка', 'Dat' => 'ку', 'Acc' => 'ка', 'Ins' => 'ком', 'Abl' => 'ке'),
    'female' => array('Gen' => 'ка', 'Dat' => 'ку', 'Acc' => 'ка', 'Ins' => 'ком', 'Abl' => 'ке'),
  ),
  array(
    'patterns' => array('Юрец'),
    'male' => array('Gen' => 'Юрца', 'Dat' => 'Юрцу', 'Acc' => 'Юрца', 'Ins' => 'Юрцом', 'Abl' => 'Юрце'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Тореш()', 'Януш()', 'Куаныш()', 'Антош()', 'Сурадж()','Жорж()','Тадеуш()'),
    'male' => array('Gen' => 'а', 'Dat' => 'у', 'Acc' => 'а', 'Ins' => 'ем', 'Abl' => 'е'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array("Сейт-", 'Аль-'),
    'male' => 'fixed',
    'female' => 'fixed',
  ),
);

$res['surnames'] = array(
  array(
    'patterns' => array('*о', '*у', '*ы', '*э', '*ё', '*ю', '*и', '*е'),
    'male' => 'fixed',
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*я'),
    'male' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'ю', 'Ins' => 'ей', 'Abl' => 'е'),
    'female' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'ю', 'Ins' => 'ей', 'Abl' => 'е'),
  ),
  array(
    'patterns' => array('Цхака(я)', 'Баркла(я)', 'Арча(я)', 'Сана(я)', 'Бера(я)', 'Ахала(я)', 'Ка(я)', 'Цома(я)', 'Шва(я)','Пла(я)'),
    'male' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'ю', 'Ins' => 'ей', 'Abl' => 'е'),
    'female' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'ю', 'Ins' => 'ей', 'Abl' => 'е'),
  ),
  array(
    'patterns' => array('*ая'),
    'male' => array('Gen' => 'аи', 'Dat' => 'ае', 'Acc' => 'аю', 'Ins' => 'аей', 'Abl' => 'ае'),
    'female' => array('Gen' => 'ой', 'Dat' => 'ой', 'Acc' => 'ую', 'Ins' => 'ой', 'Abl' => 'ой'),
  ),
  array(
    'patterns' => array('*ж(ая)', '*ш(ая)', '*щ(ая)', '*ч(ая)', '*ц(ая)'),
    'male' => array('Gen' => 'аи', 'Dat' => 'ае', 'Acc' => 'аю', 'Ins' => 'аей', 'Abl' => 'ае'),
    'female' => array('Gen' => 'ей', 'Dat' => 'ей', 'Acc' => 'ую', 'Ins' => 'ей', 'Abl' => 'ей'),
  ),
  array(
    'patterns' => array('*яя'),
    'male' => 'fixed',
    'female' => array('Gen' => 'ей', 'Dat' => 'ей', 'Acc' => 'юю', 'Ins' => 'ей', 'Abl' => 'ей'),
  ),
  
  array(
    'patterns' => array('*ия'),
    'male' => array('Gen' => 'ии', 'Dat' => 'ии', 'Acc' => 'ию', 'Ins' => 'ией', 'Abl' => 'ии'),
    'female' => array('Gen' => 'ии', 'Dat' => 'ии', 'Acc' => 'ию', 'Ins' => 'ией', 'Abl' => 'ии'),
  ),
  array(
    'patterns' => array('Мякеля', 'Лямся', 'Талья', 'Луя', 'Рейня', 'Ростобая', 'Пелля', 'Время', 
     'Титма', 'Ковыла', 'Мантула', 'Прока', 'Олусаньа', 'Хун', 'Гетия', 'Кара', 'Ча', 'Ма', 'Ойя', 'Вака',
     'Шайя', 'Шна', 'Лукка', 'Ха', 'Ю', 'Аль'),
    'male' => 'fixed',
    'female' => 'fixed', 
  ),
  array(
    'patterns' => array('*а'),
    'male' => array('Gen' => 'ы', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'),
    'female' => array('Gen' => 'ы', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'), 
  ),
  array(
    'patterns' => array('*ов(а)', '*ев(а)', '*ёв(а)', '*ын(а)', '*ин(а)'),
    'male' => array('Gen' => 'ы', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'),
    'female' => array('Gen' => 'ой', 'Dat' => 'ой', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'ой'), 
  ),
  array(
    'patterns' => array('Швидк(а)', 'Будзинськ(а)', 'Скульск(а)'),
    'male' => array('Gen' => 'а', 'Dat' => 'у', 'Acc' => 'у', 'Ins' => 'ом', 'Abl' => 'е'),
    'female' => array('Gen' => 'ой', 'Dat' => 'ой', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'ой'), 
  ),
  array(
    'patterns' => array('Ирчишен(а)', 'Вавричен(а)', 'Бикбаув(а)'),
    'male' => array('Gen' => 'ы', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'),
    'female' => array('Gen' => 'ой', 'Dat' => 'ой', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'ой'), 
  ),
  array(
    'patterns' => array('Бов(а)', 'Сов(а)', 'Худын(а)', 'Щербин(а)', 'Калин(а)', 'Былин(а)','Медын(а)',
      'Кручин(а)', 'Молин(а)', 'Рев(а)', 'Чуприн(а)', 'Дубын(а)', 'Пузын(а)', 'Бров(а)', 'Тычин(а)', 'Бобын(а)'),
    'male' => array('Gen' => 'ы', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'),
    'female' => array('Gen' => 'ы', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'), 
  ),
  
  array(
    'patterns' => array('*у(а)', '*и(а)', '*э(а)', '*е(а)', '*ю(а)', '*а(а)', '*о(а)','*я(а)'),
    'male' => 'fixed',
    'female' => 'fixed', 
  ),
  array(
    'patterns' => array('*к(а)', '*х(а)', '*г(а)'),
    'male' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'),
    'female' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'), 
  ),
  array(
    'patterns' => array('*ш(а)', '*ж(а)', '*ч(а)', '*щ(а)'),
    'male' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ей', 'Abl' => 'е'),
    'female' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ей', 'Abl' => 'е'), 
  ),
  array(
    'patterns' => array('Кваш(а)', 'Ганж(а)', 'Уш(а)', 'Ханаш(а)', 'Кривш(а)', 'Камш(а)', 'Лукш(а)'),
    'male' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'),
    'female' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'), 
  ),
  array(
    'patterns' => array('*ца'),
    'male' => array('Gen' => 'цы', 'Dat' => 'це', 'Acc' => 'цу', 'Ins' => 'цей', 'Abl' => 'це'),
    'female' => array('Gen' => 'цы', 'Dat' => 'це', 'Acc' => 'цу', 'Ins' => 'цей', 'Abl' => 'це'), 
  ),
  array(
    'patterns' => array('Дюм(а)', 'Петип(а)', 'Кешелав(а)', 'Малек(а)', 'Рошк(а)', 'Ракш(а)', 'Гар(а)', 'Шкут(а)', 
      'Опар(а)', 'Ф(а)', 'Бег(а)', 'Туг(а)', 'Дюб(а)', 'Баркала(я)', 'Гор(я)', 'Кабей(а)', 'Харчилав(а)', 'И'),
    'male' => 'fixed',
    'female' => 'fixed' 
  ),
  array(
    'patterns' => array('*ь'),
    'male' => array('Gen' => 'я', 'Dat' => 'ю', 'Acc' => 'я', 'Ins' => 'ем', 'Abl' => 'е'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*ень', 'Уласе(нь)', 'Пиве(нь)', 'Се(нь)', 'Ме(нь)', 'Хебе(нь)', 'Ште(нь)','Дре(нь)',
       'Дере(нь)', 'Липе(нь)', 'Дубе(нь)', 'Равге(нь)', 'Пиве(нь)', 'Ле(нь)', 'Цире(нь)', 'Говоре(нь)','Петре(нь)',
       'Ступе(нь)', 'Фе(нь)', 'Куте(нь)', 'Бе(нь)', 'Лазбе(нь)', 'Хитре(нь)', 'Кре(нь)', 'Ше(нь)', 'Невге(нь)'), // any "*ень" like "Уласень"? // Alexey: except monosyllabic, I guess
    'male' => array('Gen' => 'ня', 'Dat' => 'ню', 'Acc' => 'ня', 'Ins' => 'нем', 'Abl' => 'не'),
    'female' => 'fixed'
  ),
//  array(
//    'patterns' => array('Журав(ель)'),
//    'male' => array('Gen' => 'ля', 'Dat' => 'лю', 'Acc' => 'ля', 'Ins' => 'лем', 'Abl' => 'ле'),
//    'female' => 'fixed'
//  ),
  array(
    'patterns' => array('*й', 'Берего(й)', 'Водосто(й)', 'Корро(й)', 'Коро(й)', 'Геро(й)', 'Стро(й)', 
      'Алло(й)', 'Градобо(й)', 'Драпо(й)', 'Тро(й)', 'Трибо(й)', 'Килиго(й)', 'Устро(й)', 'Рокджо(й)', 'Бо(й)', 
      'Во(й)', 'Го(й)', 'До(й)', 'Ко(й)', 'Ло(й)', 'Но(й)', 'Ро(й)', 'Со(й)', 
      'То(й)', 'Ко(й)', 'Ло(й)', 'Уо(й)', 'Фо(й)', 'Хо(й)', 'Цо(й)', 'Чо(й)', 'Шо(й)', 'Забо(й)', 'Фро(й)',
      'Свинобо(й)', 'Козодо(й)', 'Волобо(й)', 'Нагисо(й)'),
    'male' => array('Gen' => 'я', 'Dat' => 'ю', 'Acc' => 'я', 'Ins' => 'ем', 'Abl' => 'е'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*ой', '*ый'),
    'male' => array('Gen' => 'ого', 'Dat' => 'ому', 'Acc' => 'ого', 'Ins' => 'ым', 'Abl' => 'ом'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*цый'),
    'male' => array('Gen' => 'цего', 'Dat' => 'цему', 'Acc' => 'цего', 'Ins' => 'цым', 'Abl' => 'цем'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*г(ой)', '*к(ой)', '*х(ой)', '*ж(ой)', '*ш(ой)', '*щ(ой)', '*ч(ой)'),
    'male' => array('Gen' => 'ого', 'Dat' => 'ому', 'Acc' => 'ого', 'Ins' => 'им', 'Abl' => 'ом'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*ий'),
    'male' => array('Gen' => 'ия', 'Dat' => 'ию', 'Acc' => 'ия', 'Ins' => 'ием', 'Abl' => 'ии'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*г(ий)', '*к(ий)', '*х(ий)'),
    'male' => array('Gen' => 'ого', 'Dat' => 'ому', 'Acc' => 'ого', 'Ins' => 'им', 'Abl' => 'ом'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*ч(ий)', '*ж(ий)', '*ш(ий)', '*щ(ий)', '*н(ий)'),
    'male' => array('Gen' => 'его', 'Dat' => 'ему', 'Acc' => 'его', 'Ins' => 'им', 'Abl' => 'ем'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('Арсени(й)', 'Гуржи(й)', 'Трухни(й)', 'Мохи(й)', 'Топчи(й)', 'Багри(й)', 'Тульчи(й)', 
      'Саланжи(й)', 'Таргони(й)', 'Стогни(й)', 'Оги(й)', "Сал\xb3(й)", 'Сысо(й)', 'Сми(й)', 'Черни(й)',
      'Гайтанжи(й)', 'Малани(й)','Компани(й)','Сырги(й)','Боки(й)', 'Копи(й)'),
    'male' => array('Gen' => 'я', 'Dat' => 'ю', 'Acc' => 'я', 'Ins' => 'ем', 'Abl' => 'и'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*ей'),
    'male' => array('Gen' => 'ея', 'Dat' => 'ею', 'Acc' => 'ея', 'Ins' => 'еем', 'Abl' => 'ее'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('Солов(ей)', 'Вороб(ей)'),
    'male' => array('Gen' => 'ья', 'Dat' => 'ью', 'Acc' => 'ья', 'Ins' => 'ьем', 'Abl' => 'ье'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('Крот', 'Сук', 'Гнеп', 'Кремс', 'Чан', 'Фа', 'Куст', 'Горох', 'Рысь', 'Лысь', 'Ша','А','О','У','Ухань','И'),
    'male' => 'fixed',
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('*их', '*ых'),
    'male' => 'fixed',
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('*ок', '*ек', '*ч(ёк)', 'Коновалён(ак)'),
    'male' => array('Gen' => 'ка', 'Dat' => 'ку', 'Acc' => 'ка', 'Ins' => 'ком', 'Abl' => 'ке'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Войтише(к)', 'Цве(к)', 'Дуде(к)', 'Буче(к)', 'Ляше(к)', 'Ше(к)', 'Клаче(к)', 'Баче(к)',
      'Корчмаре(к)', 'Дяче(к)', 'Цумбе(к)', 'Собе-Пане(к)', 'Штре(к)', 'Сире(к)', 'Псуе(к)', 'Матие(к)', 'Пане(к)', 
      'Поле(к)', 'Саде(к)', 'Стро(к)', 'Ско(к)', 'Смо(к)', 'Воло(к)', 'Набо(к)', 'Бро(к)', 'Шейнро(к)', 'Скаче(к)', 
      'Гае(к)', 'Бо(к)', 'Бло(к)', 'Кониче(к)', 'Дже(к)', 'Козодро(к)', 'Гло(к)', 'Сте(к)', 'Проро(к)', 'Чапе(к)',
      'Домаше(к)', 'Ходаче(к)', 'Гре(к)', 'Гуче(к)', 'Волче(к)', 'Дро(к)', 'Клембе(к)', 'Гайдее(к)', 'Кулаче(к)','Филобо(к)',
      'Кириче(к)', 'Ваце(к)', 'Херте(к)', 'Бе(к)', 'Конюше(к)', 'Ге(к)', 'Даутбе(к)', 'Саче(к)', 'Кло(к)','Долейше(к)','Айдарбе(к)',
      'Ро(к)', 'Кре(к)', 'Ерме(к)', 'Дане(к)', 'Быче(к)', 'Краснощё(к)', 'Здро(к)', 'Недосе(к)', 'Синео(к)','Фо(к)','Шо(к)',
      'Подрабине(к)', 'Боче(к)', 'Саме(к)', 'Фле(к)', 'Эрюре(к)', 'Кряче(к)', 'Вольче(к)', 'Заране(к)','Лоше(к)','Гле(к)',
      'Куле(к)', 'Рабе(к)', 'Заране(к)', 'Зброже(к)', 'Доброско(к)', 'Хлопче(к)', 'Бре(к)', 'Чиле(к)', 'Вале(к)', 'Шне(к)',
      'Лодбро(к)', 'Але(к)', 'Теле(к)', 'Але(к)', 'Лещео(к)', 'Куре(к)', 'Смерче(к)', 'Здро(к)','Саросе(к)','Крюче(к)','Толче(к)',
      'Жерносе(к)', 'Толо(к)', 'Лое(к)', 'Боже(к)', 'Бабе(к)', 'Тышле(к)','Крсе(к)','Пате(к)','Геро(к)','Чесно(к)','Малоо(к)'),
    'male' => array('Gen' => 'ка', 'Dat' => 'ку', 'Acc' => 'ка', 'Ins' => 'ком', 'Abl' => 'ке'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('*а(ёк)', '*о(ёк)', '*у(ёк)', '*е(ёк)', '*и(ёк)'),
    'male' => array('Gen' => 'йка', 'Dat' => 'йку', 'Acc' => 'йка', 'Ins' => 'йком', 'Abl' => 'йке'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('*ёк'),
    'male' => array('Gen' => 'ька', 'Dat' => 'ьку', 'Acc' => 'ька', 'Ins' => 'ьком', 'Abl' => 'ьке'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('*ец'),
    'male' => array('Gen' => 'ца', 'Dat' => 'цу', 'Acc' => 'ца', 'Ins' => 'цем', 'Abl' => 'це'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Жуклин(ец)', 'Гапан(ец)', 'Кремен(ец)', 'Ворон(ец)',
      'Рухов(ец)', 'От(ец)', 'Чигрин(ец)', 'Тестел(ец)', 'Короб(ец)', 'Лубен(ец)', 'Краве(ц)', 'Шв(ец)', 'Жн(ец)',
      'Бо(ец)', 'Титов(ец)', 'Скреб(ец)', 'Канив(ец)', 'Митьков(ец)', 'Зимов(ец)', 'Мул(ец)', 'Дон(ец)', 'Сидор(ец)', 'Туров(ец)',
      'Ливин(ец)', 'Чуднов(ец)', 'Мыслив(ец)', 'Бел(ец)', 'Малахов(ец)', 'Козуб(ец)', 'Казан(ец)', 'Якуб(ец)', 'Козин(ец)', 'Москал(ец)',
      'Шабан(ец)', 'Корни(ец)', 'Степан(ец)', 'Брагин(ец)', 'Левин(ец)', 'Руб(ец)', 'Кацав(ец)', 'Остап(ец)', 'Гороб(ец)',
      'Волын(ец)', 'Адын(ец)', 'Сив(ец)', 'Мелехов(ец)', 'Кор(ец)', 'Кре(ц)', 'Хлеб(ец)', 'Сидоров(ец)',
      'Сте(ц)', 'Березин(ец)', 'Москов(ец)', 'Зем(ец)', 'Редков(ец)', 'Черне(ц)', 'Крржев(ец)','Баков(ец)','Демьян(ец)',
      'Голын(ец)', 'Домов(ец)', 'Писар(ец)', 'Мисов(ец)', 'Куп(ец)', 'Пилип(ец)', 'Крив(ец)','Скомор(ец)',
      'Мошен(ец)', 'Гаврилов(ец)', 'Дун(ец)', 'Марков(ец)', 'Лесков(ец)', 'Сан(ец)', 'Слив(ец)', 'Мышков(ец)', 
      'Ум(ец)', 'Караку(ц)', 'Вежнов(ец)', 'Михнов(ец)', 'Ребков(ец)', 'Белев(ец)', 'Стругов(ец)', 'Лукьян(ец)',
      'Ферен(ец)', 'Медлов(ец)', 'Жуков(ец)', 'Полтав(ец)', 'Ляхов(ец)', 'Максим(ец)', 'Чернов(ец)', 'Товпен(ец)',
      'Конон(ец)', 'Яким(ец)', 'Обушве(ц)', 'Мале(ц)', 'Салтов(ец)', 'Стрил(ец)', 'Раков(ец)', 'Харма(ц)', 'Ман(ец)',
      'Абле(ц)', 'Кудре(ц)', 'Ше(ц)', 'Антон(ец)', 'Близне(ц)', 'Липов(ец)', 'Пе(ц)', 'Федор(ец)', 'Скобле(ц)', 'Ве(ц)',
      'Краве(ц)', 'Не(ц)','Лисив(ец)','Бегун(ец)','Кобе(ц)','Хин(ец)','Семен(ец)','Карпе(ц)','Роман(ец)'),
    'male' => array('Gen' => 'ца', 'Dat' => 'цу', 'Acc' => 'ца', 'Ins' => 'цом', 'Abl' => 'це'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Е(лец)', 'Моска(лец)', 'Стре(лец)'),
    'male' => array('Gen' => 'льца', 'Dat' => 'льцу', 'Acc' => 'льца', 'Ins' => 'льцом', 'Abl' => 'льце'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('*лец'),
    'male' => array('Gen' => 'льца', 'Dat' => 'льцу', 'Acc' => 'льца', 'Ins' => 'льцем', 'Abl' => 'льце'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('*а(ец)', '*е(ец)', '*и(ец)', '*о(ец)', '*у(ец)', '*ю(ец)', '*я(ец)', '*ы(ец)'),
    'male' => array('Gen' => 'йца', 'Dat' => 'йцу', 'Acc' => 'йца', 'Ins' => 'йцем', 'Abl' => 'йце'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Бо(ец)','Буга(ец)'),
    'male' => array('Gen' => 'йца', 'Dat' => 'йцу', 'Acc' => 'йца', 'Ins' => 'йцом', 'Abl' => 'йце'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Шв(ец)', 'Жн(ец)', 'За(ец)', 'Б(ец)', 'Бразн(ец)', 'Гр(ец)', 'Ойн(ец)', 'Хейф(ец)', 'Х(ец)','Корни(ец)'),
    'male' => array('Gen' => 'еца', 'Dat' => 'ецу', 'Acc' => 'еца', 'Ins' => 'ецом', 'Abl' => 'еце'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Кеб(ец)', 'Ге(ец)', 'Км(ец)', 'Г(ец)', 'М(ец)', 'Пр(ец)'),
    'male' => array('Gen' => 'еца', 'Dat' => 'ецу', 'Acc' => 'еца', 'Ins' => 'ецем', 'Abl' => 'еце'), // Ins не такой, как в предыдущем паттерне
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Вет(ер)'),
    'male' => array('Gen' => 'ра', 'Dat' => 'ру', 'Acc' => 'ра', 'Ins' => 'ром', 'Abl' => 'ре'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Хох(ол)'),
    'male' => array('Gen' => 'ла', 'Dat' => 'лу', 'Acc' => 'ла', 'Ins' => 'лом', 'Abl' => 'ле'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('*ёл'),
    'male' => array('Gen' => 'ла', 'Dat' => 'лу', 'Acc' => 'ла', 'Ins' => 'лом', 'Abl' => 'ле'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Анё(л)', 'Псё(л)', 'Рокотё(л)'),
    'male' => array('Gen' => 'ла', 'Dat' => 'лу', 'Acc' => 'ла', 'Ins' => 'лом', 'Abl' => 'ле'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Лев'),
    'male' => array('Gen' => 'Льва', 'Dat' => 'Льву', 'Acc' => 'Льва', 'Ins' => 'Львом', 'Abl' => 'Льве'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Каптёл'),
    'male' => 'fixed',
    'female' => 'fixed',
  ),
  
  array(
    'patterns' => array('*'),
    'male' => array('Gen' => 'а', 'Dat' => 'у', 'Acc' => 'а', 'Ins' => 'ом', 'Abl' => 'е'),
    'female' => 'fixed',
  ),
  
  array(
    'patterns' => array('Варкентин()', 'Эллин()', 'Мартын()', 'Лин()', 'Пингвин()', 'Цин()'),
    'male' => array('Gen' => 'а', 'Dat' => 'у', 'Acc' => 'а', 'Ins' => 'ом', 'Abl' => 'е'),
    'female' => 'fixed',
  ),

  array(
    'patterns' => array('Фризен()'),
    'male' => array('Gen' => 'а', 'Dat' => 'у', 'Acc' => 'а', 'Ins' => 'ым', 'Abl' => 'е'),
    'female' => 'fixed',
  ),

  array(
    'patterns' => array('*ев()', '*ов()', '*ёв()', '*ув()', '*ин()', '*ын()', 'Ковалышен()', 
       'Процкив()', 'Гнатив()', 'Павлив()', 'Котив()'), // *ув?
    'male' => array('Gen' => 'а', 'Dat' => 'у', 'Acc' => 'а', 'Ins' => 'ым', 'Abl' => 'е'),
    'female' => 'fixed',
  ),

  array(
    'patterns' => array('*ч()', '*ш()', '*ц()', '*щ()', '*ж()'),
    'male' => array('Gen' => 'а', 'Dat' => 'у', 'Acc' => 'а', 'Ins' => 'ем', 'Abl' => 'е'),
    'female' => 'fixed',
  ),

  array(
    'patterns' => array('Сыч()', 'Карандаш()', 'Чиж()', 'Кулиш()', 'Пыж()', 'Барабаш()', 'Пархач()', 
     'Деркач()', 'Грин()', 'Шин()', 'Кин()'),
    'male' => array('Gen' => 'а', 'Dat' => 'у', 'Acc' => 'а', 'Ins' => 'ом', 'Abl' => 'е'),
    'female' => 'fixed',
  ),
  
  array(
    'patterns' => array("Тер-", "Нор-", "Сулима-", "Бей-", "Джан-", "Гаген-", "Крым-", "И-", "ген-", 
      "Догуй-", "Кызыл-", "Аль-", "Шангыр-", "-оол()", "Бадма-", "дер-", "Ван-", "Ван-дер-", "Ага-", "ча-","Юн-"),
    'male' => 'fixed',
    'female' => 'fixed',
  ),
 
);

      $flex_config = $res;
    } else if (in_array($lang_id, array(1, 2, 21, 51, 91, 100))) {
switch($lang_id){

case 1:

// Alexey, 28.11.2009: настоящее украинское склонение, исправленное и дополненное
// \xb2 - I украинское, \xb3 - i украинское (портятся фаром)

$res['flexible_symbols'] = "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЬЭЮЯабвгдеёжзийклмнопрстуфхцчшщьэюя\xb2\xb3";

$res['names'] = array(
  array(
    'patterns' => array('*у', '*ы', '*э', '*ё', '*ю', '*и', '*е'),
    'male' => 'fixed',
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*я'),
    'male' => array('Gen' => 'i', 'Dat' => 'i', 'Acc' => 'ю', 'Ins' => 'ею', 'Abl' => 'i'),
    'female' => array('Gen' => 'i', 'Dat' => 'i', 'Acc' => 'ю', 'Ins' => 'ею', 'Abl' => 'i'),
  ),
  array(
    'patterns' => array("*\xb3(я)"),
    'male' => array('Gen' => 'ї', 'Dat' => 'ї', 'Acc' => 'ю', 'Ins' => 'єю', 'Abl' => 'ї'),
    'female' => array('Gen' => 'ї', 'Dat' => 'ї', 'Acc' => 'ю', 'Ins' => 'єю', 'Abl' => 'ї'),
  ),
  array(
    'patterns' => array('*о'),
    'male' => array('Gen' => 'а', 'Dat' => 'овi', 'Acc' => 'а', 'Ins' => 'ом', 'Abl' => 'овi'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('*а'),
    'male' => array('Gen' => 'и', 'Dat' => 'i', 'Acc' => 'у', 'Ins' => 'ою', 'Abl' => 'i'),
    'female' => array('Gen' => 'и', 'Dat' => 'i', 'Acc' => 'у', 'Ins' => 'ою', 'Abl' => 'i'),
  ),
  array(
    'patterns' => array('*ш(а)', '*ж(а)', '*ч(а)', '*щ(а)'),
    'male' => array('Gen' => 'i', 'Dat' => 'i', 'Acc' => 'у', 'Ins' => 'ею', 'Abl' => 'i'),
    'female' => array('Gen' => 'i', 'Dat' => 'i', 'Acc' => 'у', 'Ins' => 'ею', 'Abl' => 'i'),
  ),
  array(
    'patterns' => array('*ц(а)'),
    'male' => array('Gen' => 'и', 'Dat' => 'i', 'Acc' => 'у', 'Ins' => 'ею', 'Abl' => 'i'),
    'female' => array('Gen' => 'и', 'Dat' => 'i', 'Acc' => 'у', 'Ins' => 'ею', 'Abl' => 'i'),
  ),
  array(
    'patterns' => array('*(ка)'),
    'male' => array('Gen' => 'ки', 'Dat' => 'цi', 'Acc' => 'ку', 'Ins' => 'кою', 'Abl' => 'цi'),
    'female' => array('Gen' => 'ки', 'Dat' => 'цi', 'Acc' => 'ку', 'Ins' => 'кою', 'Abl' => 'цi'),
  ),
  array(
    'patterns' => array('*(га)'),
    'male' => array('Gen' => 'ги', 'Dat' => 'зi', 'Acc' => 'гу', 'Ins' => 'гою', 'Abl' => 'зi'),
    'female' => array('Gen' => 'ги', 'Dat' => 'зi', 'Acc' => 'гу', 'Ins' => 'гою', 'Abl' => 'зi'),
  ),
  array(
    'patterns' => array('*(ха)'),
    'male' => array('Gen' => 'хи', 'Dat' => 'сi', 'Acc' => 'ху', 'Ins' => 'хою', 'Abl' => 'сi'),
    'female' => array('Gen' => 'хи', 'Dat' => 'сi', 'Acc' => 'ху', 'Ins' => 'хою', 'Abl' => 'сi'),
  ),
  array(
    'patterns' => array('*й'),
    'male' => array('Gen' => 'я', 'Dat' => 'ю', 'Acc' => 'я', 'Ins' => 'єм', 'Abl' => 'ї'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*ь'),
    'male' => array('Gen' => 'я', 'Dat' => 'евi', 'Acc' => 'я', 'Ins' => 'ем', 'Abl' => 'евi'),
    'female' => array('Gen' => 'i', 'Dat' => 'i', 'Acc' => 'ь', 'Ins' => 'ю', 'Abl' => 'i')
  ),
  array(
    'patterns' => array('*ёк'),
    'male' => array('Gen' => 'ька', 'Dat' => 'ьковi', 'Acc' => 'ька', 'Ins' => 'ьком', 'Abl' => 'ьковi'),
    'female' => array('Gen' => 'ька', 'Dat' => 'ьковi', 'Acc' => 'ька', 'Ins' => 'ьком', 'Abl' => 'ьковi'),
  ),
  array(
    'patterns' => array('*ок'),
    'male' => array('Gen' => 'ка', 'Dat' => 'ковi', 'Acc' => 'ка', 'Ins' => 'ком', 'Abl' => 'ковi'),
    'female' => array('Gen' => 'ка', 'Dat' => 'ковi', 'Acc' => 'ка', 'Ins' => 'ком', 'Abl' => 'ковi'),
  ),
  array(
    'patterns' => array('*'),
    'male' => array('Gen' => 'а', 'Dat' => 'овi', 'Acc' => 'а', 'Ins' => 'ом', 'Abl' => 'овi'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array("\xb2гор"),
    'male' => array('Gen' => "\xb2горя", 'Dat' => "\xb2горевi", 'Acc' => "\xb2горя", 'Ins' => "\xb2горем", 'Abl' => "Iгоревi"),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Гюзель', 'Гузель', 'Айгуль', 'Айгюль', 'Гюнэль', 'Гюнель', 'Даниэль', 'Аревик',
'Астхик', 'Шагик', 'Татевик', 'Сатеник', 'Манушак', 'Анушик', 'Хасмик', 'Назик', 'Кайцак', 'Ардак', 'Арпик',
'Гюзяль', 'Жибек', 'Асель', 'Николь', 'Аниель', 'Асмик', 'Ола'),
    'male' => 'fixed',
    'female' => 'fixed'
  ),

  array(
    'patterns' => array('*ий'),
    'male' => array('Gen' => 'iя', 'Dat' => 'iю', 'Acc' => 'iя', 'Ins' => 'iєм', 'Abl' => 'iї'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*ия'),
    'male' => array('Gen' => 'iї', 'Dat' => 'iї', 'Acc' => 'iю', 'Ins' => 'iєю', 'Abl' => 'iї'),
    'female' => array('Gen' => 'iї', 'Dat' => 'iї', 'Acc' => 'iю', 'Ins' => 'iєю', 'Abl' => 'iї'),
  ),
  array(
    'patterns' => array('Пётр'),
    'male' => array('Gen' => 'Петра', 'Dat' => 'Петровi', 'Acc' => 'Петра', 'Ins' => 'Петром', 'Abl' => 'Петровi'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Павел'),
    'male' => array('Gen' => 'Павла', 'Dat' => 'Павловi', 'Acc' => 'Павла', 'Ins' => 'Павлом', 'Abl' => 'Павловi'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Лев'),
    'male' => array('Gen' => 'Льва', 'Dat' => 'Львовi', 'Acc' => 'Льва', 'Ins' => 'Львом', 'Abl' => 'Львовi'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('*а(ёк)', '*о(ёк)', '*у(ёк)', '*е(ёк)', '*и(ёк)'),
    'male' => array('Gen' => 'йка', 'Dat' => 'йковi', 'Acc' => 'йка', 'Ins' => 'йком', 'Abl' => 'йковi'),
    'female' => array('Gen' => 'йка', 'Dat' => 'йковi', 'Acc' => 'йка', 'Ins' => 'йком', 'Abl' => 'йковi'),
  ),
  array(
    'patterns' => array('Юрец'),
    'male' => array('Gen' => 'Юрца', 'Dat' => 'Юрцевi', 'Acc' => 'Юрца', 'Ins' => 'Юрцем', 'Abl' => 'Юрцевi'),
    'female' => 'fixed',
  ),
);


$res['surnames'] = array(
  array(
    'patterns' => array('*у', '*ы', '*э', '*ё', '*ю', '*и', '*е'),
    'male' => 'fixed',
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*о'),
    'male' => array('Gen' => 'а', 'Dat' => 'овi', 'Acc' => 'а', 'Ins' => 'ом', 'Abl' => 'овi'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*я'),
    'male' => array('Gen' => 'i', 'Dat' => 'i', 'Acc' => 'ю', 'Ins' => 'ею', 'Abl' => 'i'),
    'female' => array('Gen' => 'i', 'Dat' => 'i', 'Acc' => 'ю', 'Ins' => 'ею', 'Abl' => 'i'),
  ),
  array(
    'patterns' => array("*\xb3(я)"),
    'male' => array('Gen' => 'ї', 'Dat' => 'ї', 'Acc' => 'ю', 'Ins' => 'єю', 'Abl' => 'ї'),
    'female' => array('Gen' => 'ї', 'Dat' => 'ї', 'Acc' => 'ю', 'Ins' => 'єю', 'Abl' => 'ї'),
  ),
  array(
    'patterns' => array('Цхака(я)', 'Баркла(я)', 'Арча(я)', 'Сана(я)'),
    'male' => array('Gen' => 'ї', 'Dat' => 'ї', 'Acc' => 'ю', 'Ins' => 'єю', 'Abl' => 'ї'),
    'female' => array('Gen' => 'ї', 'Dat' => 'ї', 'Acc' => 'ю', 'Ins' => 'єю', 'Abl' => 'ї'),
  ),
  array(
    'patterns' => array('*ая'),
    'male' => array('Gen' => 'аї', 'Dat' => 'аї', 'Acc' => 'аю', 'Ins' => 'аєю', 'Abl' => 'аї'),
    'female' => array('Gen' => 'ої', 'Dat' => 'ої', 'Acc' => 'ую', 'Ins' => 'ою', 'Abl' => 'ої'),
  ),
  array(
    'patterns' => array('*ий'),
    'male' => array('Gen' => 'iя', 'Dat' => 'iю', 'Acc' => 'iя', 'Ins' => 'iєм', 'Abl' => 'iї'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*ия'),
    'male' => array('Gen' => 'iї', 'Dat' => 'iї', 'Acc' => 'iю', 'Ins' => 'iєю', 'Abl' => 'iї'),
    'female' => array('Gen' => 'iї', 'Dat' => 'iї', 'Acc' => 'iю', 'Ins' => 'iєю', 'Abl' => 'iї'),
  ),
  array(
    'patterns' => array('*а'),
    'male' => array('Gen' => 'и', 'Dat' => 'i', 'Acc' => 'у', 'Ins' => 'ою', 'Abl' => 'i'),
    'female' => array('Gen' => 'и', 'Dat' => 'i', 'Acc' => 'у', 'Ins' => 'ою', 'Abl' => 'i'),
  ),
  array(
    'patterns' => array('*ш(а)', '*ж(а)', '*ч(а)', '*щ(а)'),
    'male' => array('Gen' => 'i', 'Dat' => 'i', 'Acc' => 'у', 'Ins' => 'ею', 'Abl' => 'i'),
    'female' => array('Gen' => 'i', 'Dat' => 'i', 'Acc' => 'у', 'Ins' => 'ею', 'Abl' => 'i'),
  ),
  array(
    'patterns' => array('*ц(а)'),
    'male' => array('Gen' => 'и', 'Dat' => 'i', 'Acc' => 'у', 'Ins' => 'ею', 'Abl' => 'i'),
    'female' => array('Gen' => 'и', 'Dat' => 'i', 'Acc' => 'у', 'Ins' => 'ею', 'Abl' => 'i'),
  ),
  array(
    'patterns' => array('*(ка)'),
    'male' => array('Gen' => 'ки', 'Dat' => 'цi', 'Acc' => 'ку', 'Ins' => 'кою', 'Abl' => 'цi'),
    'female' => array('Gen' => 'ки', 'Dat' => 'цi', 'Acc' => 'ку', 'Ins' => 'кою', 'Abl' => 'цi'),
  ),
  array(
    'patterns' => array('*(га)'),
    'male' => array('Gen' => 'ги', 'Dat' => 'зi', 'Acc' => 'гу', 'Ins' => 'гою', 'Abl' => 'зi'),
    'female' => array('Gen' => 'ги', 'Dat' => 'зi', 'Acc' => 'гу', 'Ins' => 'гою', 'Abl' => 'зi'),
  ),
  array(
    'patterns' => array('*(ха)'),
    'male' => array('Gen' => 'хи', 'Dat' => 'сi', 'Acc' => 'ху', 'Ins' => 'хою', 'Abl' => 'сi'),
    'female' => array('Gen' => 'хи', 'Dat' => 'сi', 'Acc' => 'ху', 'Ins' => 'хою', 'Abl' => 'сi'),
  ),
  array(
    'patterns' => array('*й'),
    'male' => array('Gen' => 'я', 'Dat' => 'ю', 'Acc' => 'я', 'Ins' => 'єм', 'Abl' => 'ї'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*ь', 'Кобзар()'),
    'male' => array('Gen' => 'я', 'Dat' => 'евi', 'Acc' => 'я', 'Ins' => 'ем', 'Abl' => 'евi'),
    'female' => 'fixed'
    //'female' => array('Gen' => 'i', 'Dat' => 'i', 'Acc' => 'ь', 'Ins' => 'ю', 'Abl' => 'i')
  ),
  array(
    'patterns' => array('*ов(а)', '*ев(а)', '*ёв(а)', '*ын(а)', '*ин(а)', 'Бруцьк(а)'),
    'male' => array('Gen' => 'и', 'Dat' => 'i', 'Acc' => 'у', 'Ins' => 'ою', 'Abl' => 'i'),
    'female' => array('Gen' => 'ої', 'Dat' => 'iй', 'Acc' => 'у', 'Ins' => 'ою', 'Abl' => 'iй'), 
  ),
  array(
    'patterns' => array('Бов(а)', 'Сов(а)', 'Худын(а)', 'Щербин(а)', 'Калин(а)'),
    'male' => array('Gen' => 'и', 'Dat' => 'i', 'Acc' => 'у', 'Ins' => 'ою', 'Abl' => 'i'),
    'female' => array('Gen' => 'и', 'Dat' => 'i', 'Acc' => 'у', 'Ins' => 'ою', 'Abl' => 'i'), 
  ),
  
  array(
    'patterns' => array('*у(а)', '*и(а)', '*э(а)', '*е(а)', '*ю(а)'),
    'male' => 'fixed',
    'female' => 'fixed', 
  ),
  array(
    'patterns' => array('Дюм(а)', 'Петип(а)', 'Кешелав(а)', 'Малек(а)', 'Рошк(а)', 'Ракш(а)', 'Гар(а)', 'Шкут(а)', 'Опар(а)', 'Ф(а)', 'Бег(а)'),
    'male' => 'fixed',
    'female' => 'fixed' 
  ),
  array(
    'patterns' => array('*ень', 'Уласе(нь)', 'Пиве(нь)','Се(нь)'),
    'male' => array('Gen' => 'ня', 'Dat' => 'ню', 'Acc' => 'ня', 'Ins' => 'нем', 'Abl' => 'невi'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*й', 'Берего(й)', 'Водосто(й)', 'Корро(й)', 'Коро(й)', 'Геро(й)', 'Стро(й)', 'Алло(й)', 
       'Градобо(й)', 'Бо(й)', 'Во(й)', 'Го(й)', 'До(й)', 'Ко(й)', 'Ло(й)', 'Но(й)', 'Ро(й)', 'Со(й)', 
       'То(й)', 'Ко(й)', 'Ло(й)', 'Уо(й)', 'Фо(й)', 'Хо(й)', 'Цо(й)', 'Чо(й)', 'Шо(й)', "Гармат\xb3(й)",
       "Гуз\xb3(й)", 'Рокджо(й)', "См\xb3(й)", 'Сми(й)',"Сал\xb3(й)"),
    'male' => array('Gen' => 'я', 'Dat' => 'ю', 'Acc' => 'я', 'Ins' => 'єм', 'Abl' => 'ї'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*ой', '*ый'),
    'male' => array('Gen' => 'ого', 'Dat' => 'ому', 'Acc' => 'ого', 'Ins' => 'им', 'Abl' => 'iм'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*цый'),
    'male' => array('Gen' => 'цего', 'Dat' => 'цему', 'Acc' => 'цего', 'Ins' => 'цим', 'Abl' => 'цiм'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*(гой)', '*(гий)'),
    'male' => array('Gen' => 'гого', 'Dat' => 'гому', 'Acc' => 'гого', 'Ins' => 'гим', 'Abl' => 'зiм'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*(кой)', '*(кий)'),
    'male' => array('Gen' => 'кого', 'Dat' => 'кому', 'Acc' => 'кого', 'Ins' => 'ким', 'Abl' => 'цiм'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*(хой)', '*(хий)'),
    'male' => array('Gen' => 'хого', 'Dat' => 'хому', 'Acc' => 'хого', 'Ins' => 'хим', 'Abl' => 'сiм'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*ий'),
    'male' => array('Gen' => 'ого', 'Dat' => 'ому', 'Acc' => 'ого', 'Ins' => 'им', 'Abl' => 'iм'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*ч(ий)', '*ж(ий)', '*ш(ий)', '*щ(ий)'),
    'male' => array('Gen' => 'его', 'Dat' => 'ему', 'Acc' => 'его', 'Ins' => 'им', 'Abl' => 'iм'),
    'female' => 'fixed'
  ),
  array(
   'patterns' => array('Арсени(й)', 'Гуржи(й)', 'Трухни(й)', 'Мохи(й)', 'Топчи(й)', "Пал\xb3(й)", "Баб\xb3(й)", "Бабi(й)","Крас\xb3(й)",'Красiй',
      "Багр\xb3(й)","Багрi(й)","Базал\xb3(й)","Базалi(й)","Кул\xb3(й)",'Кулiй',"Г\xb3(й)",'Гiй',"Гур\xb3(й)",'Гурiй','Чернiй',"Черн\xb3(й)"),
    'male' => array('Gen' => 'я', 'Dat' => 'ю', 'Acc' => 'я', 'Ins' => 'єм', 'Abl' => 'ї'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array("*\xb3й", 'Скотн(ий)'),
    'male' => array('Gen' => 'ього', 'Dat' => 'ьому', 'Acc' => 'ього', 'Ins' => 'iм', 'Abl' => 'iм'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array("Дупл\xb3(й)", "Микит\xb3(й)"),
    'male' => array('Gen' => "я", 'Dat' => "ю", 'Acc' => "я", 'Ins' => "єм", 'Abl' => "ї"),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*ей'),
    'male' => array('Gen' => 'ея', 'Dat' => 'ею', 'Acc' => 'ея', 'Ins' => 'еєм', 'Abl' => 'еї'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('Солов(ей)', 'Вороб(ей)'),
    'male' => array('Gen' => '&#39;я', 'Dat' => '&#39;ю', 'Acc' => '&#39;я', 'Ins' => '&#39;єм', 'Abl' => '&#39;ї'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*их', '*ых'),
    'male' => 'fixed',
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Лок(оть)'),
    'male' => array('Gen' => 'тя', 'Dat' => 'тевi', 'Acc' => 'тя', 'Ins' => 'тем', 'Abl' => 'тевi'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array("К(\xb3т)",'К(iт)'),
    'male' => array('Gen' => 'ота', 'Dat' => 'оту', 'Acc' => 'ота', 'Ins' => 'отом', 'Abl' => 'отовi'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('*ок', '*ек', '*ч(ёк)'),
    'male' => array('Gen' => 'ка', 'Dat' => 'ковi', 'Acc' => 'ка', 'Ins' => 'ком', 'Abl' => 'ковi'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('*ёк'),
    'male' => array('Gen' => 'ька', 'Dat' => 'ьковi', 'Acc' => 'ька', 'Ins' => 'ьком', 'Abl' => 'ьковi'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Войтише(к)', 'Цве(к)', 'Дуде(к)', 'Буче(к)', 'Ляше(к)', 'Ше(к)', 'Клаче(к)', 'Баче(к)','Маче(к)',
      'Корчмаре(к)', 'Дяче(к)', 'Цумбе(к)', 'Собе-Пане(к)', 'Штре(к)', 'Сире(к)', 'Псуе(к)', 'Матие(к)', 'Пане(к)', 'Поле(к)', 'Саде(к)', 'Стро(к)', 'Ско(к)', 'Смо(к)', 'Воло(к)', 'Набо(к)', 'Бро(к)'),
    'male' => array('Gen' => 'ка', 'Dat' => 'ковi', 'Acc' => 'ка', 'Ins' => 'ком', 'Abl' => 'ковi'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('*ец'),
    'male' => array('Gen' => 'ца', 'Dat' => 'цу', 'Acc' => 'ца', 'Ins' => 'цом', 'Abl' => 'цевi'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Е(лец)'),
    'male' => array('Gen' => 'льца', 'Dat' => 'льцу', 'Acc' => 'льца', 'Ins' => 'льцом', 'Abl' => 'льцевi'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('*лец'),
    'male' => array('Gen' => 'льца', 'Dat' => 'льцу', 'Acc' => 'льца', 'Ins' => 'льцем', 'Abl' => 'льцевi'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('*а(ец)', '*е(ец)', '*и(ец)', '*о(ец)', '*у(ец)', '*ю(ец)', '*я(ец)', '*ы(ец)'),
    'male' => array('Gen' => 'йца', 'Dat' => 'йцу', 'Acc' => 'йца', 'Ins' => 'йцем', 'Abl' => 'йцевi'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array("\xb3(ець)", '*а(єць)', '*е(єць)', '*и(єць)', '*о(єць)', '*у(єць)', '*ю(єць)', '*я(єць)', '*ы(єць)'),
    'male' => array('Gen' => 'йця', 'Dat' => 'йцю', 'Acc' => 'йця', 'Ins' => 'йцем', 'Abl' => 'йцевi'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Ясков(ець)',),
    'male' => array('Gen' => 'ця', 'Dat' => 'цевi', 'Acc' => 'ця', 'Ins' => 'цем', 'Abl' => 'цевi'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('*ець'),
    'male' => array('Gen' => 'ця', 'Dat' => 'цю', 'Acc' => 'ця', 'Ins' => 'цем', 'Abl' => 'цевi'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Шв(ець)', 'Жн(ець)', 'За(ець)', 'Б(ець)', 'Бразн(ець)', 'Гр(ець)', 'Ойн(ець)', 'Хейф(ець)','Ст(ець)'),
    'male' => array('Gen' => 'еця', 'Dat' => 'ецю', 'Acc' => 'еця', 'Ins' => 'ецем', 'Abl' => 'ецевi'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Шв(ец)', 'Жн(ец)', 'За(ец)', 'Б(ец)', 'Бразн(ец)', 'Гр(ец)', 'Ойн(ец)', 'Хейф(ец)'),
    'male' => array('Gen' => 'еца', 'Dat' => 'ецу', 'Acc' => 'еца', 'Ins' => 'ецом', 'Abl' => 'ецевi'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('*ёл'),
    'male' => array('Gen' => 'ла', 'Dat' => 'лу', 'Acc' => 'ла', 'Ins' => 'лом', 'Abl' => 'ловi'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('*', 'Литвин()'),
    'male' => array('Gen' => 'а', 'Dat' => 'у', 'Acc' => 'а', 'Ins' => 'ом', 'Abl' => 'овi'),
    'female' => 'fixed',
  ),
  
  array(
    'patterns' => array('*ев()', '*ов()', '*ёв()', '*ин()', '*ын()', 'Ковалышен()'), // *ув?
    'male' => array('Gen' => 'а', 'Dat' => 'у', 'Acc' => 'а', 'Ins' => 'им', 'Abl' => 'овi'),
    'female' => 'fixed',
  ),

  array(
    'patterns' => array('*ч()', '*ш()', '*ц()', '*щ()', '*ж()'),
    'male' => array('Gen' => 'а', 'Dat' => 'у', 'Acc' => 'а', 'Ins' => 'ем', 'Abl' => 'евi'),
    'female' => 'fixed',
  ),

  array(
    'patterns' => array('Сыч()', 'Карандаш()', 'Чиж()', 'Кулиш()', 'Пыж()', 'Барабаш()', 'Пархач()', 'Деркач()', 'Грин()', 'Шин()'),
    'male' => array('Gen' => 'а', 'Dat' => 'у', 'Acc' => 'а', 'Ins' => 'ом', 'Abl' => 'евi'),
    'female' => 'fixed',
  ),
  
);

break;

case 2:

// Alexey, 29.11.2009: настоящее белорусское склонение, исправленное и дополненное
// \xb2 - I белорусское, \xb3 - i белорусское (портятся фаром)

$res['flexible_symbols'] = "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЬЭЮЯабвгдеёжзийклмнопрстуфхцчшщьэюя\xa2\xb2\xb3";

$res['names'] = array(
  array(
    'patterns' => array('*у', '*э', '*ё', '*ю', '*и', '*е'),
    'male' => 'fixed',
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*я'),
    'male' => array('Gen' => 'i', 'Dat' => 'ю', 'Acc' => 'ю', 'Ins' => 'ем', 'Abl' => 'ю'),
    'female' => array('Gen' => 'i', 'Dat' => 'i', 'Acc' => 'ю', 'Ins' => 'яй', 'Abl' => 'i')
  ),
  array(
    'patterns' => array('*г(о)', '*к(о)', '*х(о)'),
    'male' => array('Gen' => 'i', 'Dat' => 'у', 'Acc' => 'у', 'Ins' => 'ом', 'Abl' => 'у'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*о'),
    'male' => array('Gen' => 'ы', 'Dat' => 'у', 'Acc' => 'у', 'Ins' => 'ом', 'Abl' => 'у'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*ў'),
    'male' => array('Gen' => 'ва', 'Dat' => 'ву', 'Acc' => 'ва', 'Ins' => 'вам', 'Abl' => 'ву'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*й', '*ь'),
    'male' => array('Gen' => 'я', 'Dat' => 'ю', 'Acc' => 'я', 'Ins' => 'ем', 'Abl' => 'ю'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*ы'),
    'male' => array('Gen' => 'ыя', 'Dat' => 'ыю', 'Acc' => 'ыя', 'Ins' => 'ыем', 'Abl' => 'ыю'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array("*\xb3"),
    'male' => array('Gen' => 'iя', 'Dat' => 'iю', 'Acc' => 'iя', 'Ins' => 'iем', 'Abl' => 'iю'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*ка'),
    'male' => array('Gen' => 'кi', 'Dat' => 'ку', 'Acc' => 'ку', 'Ins' => 'кам', 'Abl' => 'ку'),
    'female' => array('Gen' => 'кi', 'Dat' => 'цы', 'Acc' => 'ку', 'Ins' => 'кай', 'Abl' => 'цы')
  ),
  array(
    'patterns' => array('*га'),
    'male' => array('Gen' => 'гi', 'Dat' => 'гу', 'Acc' => 'гу', 'Ins' => 'гам', 'Abl' => 'гу'),
    'female' => array('Gen' => 'гi', 'Dat' => 'зе', 'Acc' => 'гу', 'Ins' => 'гай', 'Abl' => 'зе')
  ),
  array(
    'patterns' => array('*ха'),
    'male' => array('Gen' => 'хi', 'Dat' => 'ху', 'Acc' => 'ху', 'Ins' => 'хам', 'Abl' => 'ху'),
    'female' => array('Gen' => 'хi', 'Dat' => 'се', 'Acc' => 'ху', 'Ins' => 'хай', 'Abl' => 'се')
  ),
  array(
    'patterns' => array('*ж(а)', '*ш(а)', '*ч(а)', '*ц(а)', '*р(а)'),
    'male' => array('Gen' => 'ы', 'Dat' => 'у', 'Acc' => 'у', 'Ins' => 'ам', 'Abl' => 'у'),
    'female' => array('Gen' => 'ы', 'Dat' => 'ы', 'Acc' => 'у', 'Ins' => 'ай', 'Abl' => 'ы')
  ),
  array(
    'patterns' => array('*ща'),
    'male' => array('Gen' => 'шчы', 'Dat' => 'шчу', 'Acc' => 'шчу', 'Ins' => 'шчам', 'Abl' => 'шчу'),
    'female' => array('Gen' => 'шчы', 'Dat' => 'шчы', 'Acc' => 'шчу', 'Ins' => 'шчай', 'Abl' => 'шчы')
  ),
  array(
    'patterns' => array('*ста'),
    'male' => array('Gen' => 'сты', 'Dat' => 'сту', 'Acc' => 'сту', 'Ins' => 'стам', 'Abl' => 'сту'),
    'female' => array('Gen' => 'сты', 'Dat' => 'сьце', 'Acc' => 'сту', 'Ins' => 'стай', 'Abl' => 'сьце')
  ),
  array(
    'patterns' => array('*зта'),
    'male' => array('Gen' => 'зты', 'Dat' => 'зту', 'Acc' => 'зту', 'Ins' => 'зтам', 'Abl' => 'зту'),
    'female' => array('Gen' => 'зты', 'Dat' => 'зьце', 'Acc' => 'зту', 'Ins' => 'зтай', 'Abl' => 'зьце')
  ),
  array(
    'patterns' => array('*нта'),
    'male' => array('Gen' => 'нты', 'Dat' => 'нту', 'Acc' => 'нту', 'Ins' => 'нтам', 'Abl' => 'нту'),
    'female' => array('Gen' => 'нты', 'Dat' => 'ньце', 'Acc' => 'нту', 'Ins' => 'нтай', 'Abl' => 'нзьце')
  ),
  array(
    'patterns' => array('*лта'),
    'male' => array('Gen' => 'лты', 'Dat' => 'лту', 'Acc' => 'лту', 'Ins' => 'лтам', 'Abl' => 'лту'),
    'female' => array('Gen' => 'лты', 'Dat' => 'льце', 'Acc' => 'лту', 'Ins' => 'лтай', 'Abl' => 'льце')
  ),
  array(
    'patterns' => array('*сда'),
    'male' => array('Gen' => 'сды', 'Dat' => 'сду', 'Acc' => 'сду', 'Ins' => 'сдам', 'Abl' => 'сду'),
    'female' => array('Gen' => 'сды', 'Dat' => 'сьдзе', 'Acc' => 'сду', 'Ins' => 'сдай', 'Abl' => 'сьдзе')
  ),
  array(
    'patterns' => array('*зда'),
    'male' => array('Gen' => 'зды', 'Dat' => 'зду', 'Acc' => 'зду', 'Ins' => 'здам', 'Abl' => 'зду'),
    'female' => array('Gen' => 'зды', 'Dat' => 'зьдзе', 'Acc' => 'зду', 'Ins' => 'здай', 'Abl' => 'зьдзе')
  ),
  array(
    'patterns' => array('*нда'),
    'male' => array('Gen' => 'нды', 'Dat' => 'нду', 'Acc' => 'нду', 'Ins' => 'ндам', 'Abl' => 'нду'),
    'female' => array('Gen' => 'нды', 'Dat' => 'ньдзе', 'Acc' => 'нду', 'Ins' => 'ндай', 'Abl' => 'нзьдзе')
  ),
  array(
    'patterns' => array('*лда'),
    'male' => array('Gen' => 'лды', 'Dat' => 'лду', 'Acc' => 'лду', 'Ins' => 'лдам', 'Abl' => 'лду'),
    'female' => array('Gen' => 'лды', 'Dat' => 'льдзе', 'Acc' => 'лду', 'Ins' => 'лдай', 'Abl' => 'льдзе')
  ),
  array(
    'patterns' => array('*та'),
    'male' => array('Gen' => 'ты', 'Dat' => 'ту', 'Acc' => 'ту', 'Ins' => 'там', 'Abl' => 'ту'),
    'female' => array('Gen' => 'ты', 'Dat' => 'це', 'Acc' => 'ту', 'Ins' => 'тай', 'Abl' => 'це')
  ),
  array(
    'patterns' => array('*да'),
    'male' => array('Gen' => 'ды', 'Dat' => 'ду', 'Acc' => 'ду', 'Ins' => 'дам', 'Abl' => 'ду'),
    'female' => array('Gen' => 'ды', 'Dat' => 'дзе', 'Acc' => 'ду', 'Ins' => 'дай', 'Abl' => 'дзе')
  ),
  array(
    'patterns' => array('*а'),
    'male' => array('Gen' => 'ы', 'Dat' => 'у', 'Acc' => 'у', 'Ins' => 'ам', 'Abl' => 'у'),
    'female' => array('Gen' => 'ы', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ай', 'Abl' => 'е')
  ),
  array(
    'patterns' => array('*щ'),
    'male' => array('Gen' => 'шча', 'Dat' => 'шчу', 'Acc' => 'шча', 'Ins' => 'шчам', 'Abl' => 'шчу'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*'),
    'male' => array('Gen' => 'а', 'Dat' => 'у', 'Acc' => 'а', 'Ins' => 'ам', 'Abl' => 'у'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('Пётр', 'Петр'),
    'male' => array('Gen' => 'Пятры', 'Dat' => 'Пятру', 'Acc' => 'Пятру', 'Ins' => 'Пятром', 'Abl' => 'Пятру'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('Павел', 'Павал'),
    'male' => array('Gen' => 'Паўла', 'Dat' => 'Паўлу', 'Acc' => 'Паўла', 'Ins' => 'Паўлам', 'Abl' => 'Паўлу'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('Аляксандар', 'Александр'),
    'male' => array('Gen' => 'Аляксандра', 'Dat' => 'Аляксандру', 'Acc' => 'Аляксандра', 'Ins' => 'Аляксандрам', 'Abl' => 'Аляксандру'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array("\xb2льля", "\xb2льля", "\xb2лья"),
    'male' => array('Gen' => "\xb2льлi", 'Dat' => "\xb2льлю", 'Acc' => "\xb2льлю", 'Ins' => "\xb2льлём", 'Abl' => "\xb2льлю"),
    'female' => 'fixed'
  ),
);


$res['surnames'] = array(
  array(
    'patterns' => array('*у', '*э', '*ё', '*ю', '*е'),
    'male' => 'fixed',
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*ая'),
    'male' => array('Gen' => 'аi', 'Dat' => 'аю', 'Acc' => 'аю', 'Ins' => 'аем', 'Abl' => 'аю'),
    'female' => array('Gen' => 'ай', 'Dat' => 'ай', 'Acc' => 'ую', 'Ins' => 'ай', 'Abl' => 'ай')
  ),
  array(
    'patterns' => array('*яя'),
    'male' => array('Gen' => 'яi', 'Dat' => 'яю', 'Acc' => 'яю', 'Ins' => 'яем', 'Abl' => 'яю'),
    'female' => array('Gen' => 'яй', 'Dat' => 'яй', 'Acc' => 'юю', 'Ins' => 'яй', 'Abl' => 'яй')
  ),
  array(
    'patterns' => array('*я'),
    'male' => array('Gen' => 'i', 'Dat' => 'ю', 'Acc' => 'ю', 'Ins' => 'ем', 'Abl' => 'ю'),
    'female' => array('Gen' => 'i', 'Dat' => 'i', 'Acc' => 'ю', 'Ins' => 'яй', 'Abl' => 'i')
  ),
  array(
    'patterns' => array('*йко', '*ько'),
    'male' => 'fixed',
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*ка', '*ко'),
    'male' => array('Gen' => 'кi', 'Dat' => 'ку', 'Acc' => 'ку', 'Ins' => 'кам', 'Abl' => 'ку'),
    'female' => array('Gen' => 'кi', 'Dat' => 'цы', 'Acc' => 'ку', 'Ins' => 'кай', 'Abl' => 'цы')
  ),
  array(
    'patterns' => array('*га', '*го'),
    'male' => array('Gen' => 'гi', 'Dat' => 'гу', 'Acc' => 'гу', 'Ins' => 'гам', 'Abl' => 'гу'),
    'female' => array('Gen' => 'гi', 'Dat' => 'зе', 'Acc' => 'гу', 'Ins' => 'гай', 'Abl' => 'зе')
  ),
  array(
    'patterns' => array('*ха', '*хо'),
    'male' => array('Gen' => 'хi', 'Dat' => 'ху', 'Acc' => 'ху', 'Ins' => 'хам', 'Abl' => 'ху'),
    'female' => array('Gen' => 'хi', 'Dat' => 'се', 'Acc' => 'ху', 'Ins' => 'хай', 'Abl' => 'се')
  ),
  array(
    'patterns' => array('*о'),
    'male' => array('Gen' => 'ы', 'Dat' => 'у', 'Acc' => 'у', 'Ins' => 'ом', 'Abl' => 'у'),
    'female' => array('Gen' => 'ы', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ай', 'Abl' => 'е')
  ),
  array(
    'patterns' => array('*ў'),
    'male' => array('Gen' => 'ва', 'Dat' => 'ву', 'Acc' => 'ва', 'Ins' => 'вам', 'Abl' => 'ву'),
    'female' => array('Gen' => 'ва', 'Dat' => 'ву', 'Acc' => 'ва', 'Ins' => 'вам', 'Abl' => 'ву')
  ),
  array(
    'patterns' => array('*ь'),
    'male' => array('Gen' => 'я', 'Dat' => 'ю', 'Acc' => 'я', 'Ins' => 'ем', 'Abl' => 'ю'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*ы'),
    'male' => array('Gen' => 'ага', 'Dat' => 'аму', 'Acc' => 'ага', 'Ins' => 'ым', 'Abl' => 'ым'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array("*\xb2",'*i'),
    'male' => array('Gen' => 'ага', 'Dat' => 'аму', 'Acc' => 'ага', 'Ins' => 'iм', 'Abl' => 'iм'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*и'),
    'male' => array('Gen' => 'ага', 'Dat' => 'аму', 'Acc' => 'ага', 'Ins' => 'им', 'Abl' => 'им'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*ый'),
    'male' => array('Gen' => 'ага', 'Dat' => 'аму', 'Acc' => 'ага', 'Ins' => 'ым', 'Abl' => 'ым'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*ий'),
    'male' => array('Gen' => 'ага', 'Dat' => 'аму', 'Acc' => 'ага', 'Ins' => 'им', 'Abl' => 'им'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array("*\xb3й"),
    'male' => array('Gen' => 'ага', 'Dat' => 'аму', 'Acc' => 'ага', 'Ins' => 'iм', 'Abl' => 'iм'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*ой'),
    'male' => array('Gen' => 'ога', 'Dat' => 'ому', 'Acc' => 'ога', 'Ins' => 'ом', 'Abl' => 'ом'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*й'),
    'male' => array('Gen' => 'я', 'Dat' => 'ю', 'Acc' => 'я', 'Ins' => 'ям', 'Abl' => 'ю'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*ж(а)', '*ш(а)', '*ч(а)', '*ц(а)', '*р(а)'),
    'male' => array('Gen' => 'ы', 'Dat' => 'у', 'Acc' => 'у', 'Ins' => 'ам', 'Abl' => 'у'),
    'female' => array('Gen' => 'ы', 'Dat' => 'ы', 'Acc' => 'у', 'Ins' => 'ай', 'Abl' => 'ы')
  ),
  array(
    'patterns' => array('*ща'),
    'male' => array('Gen' => 'шчы', 'Dat' => 'шчу', 'Acc' => 'шчу', 'Ins' => 'шчам', 'Abl' => 'шчу'),
    'female' => array('Gen' => 'шчы', 'Dat' => 'шчы', 'Acc' => 'шчу', 'Ins' => 'шчай', 'Abl' => 'шчы')
  ),
  array(
    'patterns' => array('*ста'),
    'male' => array('Gen' => 'сты', 'Dat' => 'сту', 'Acc' => 'сту', 'Ins' => 'стам', 'Abl' => 'сту'),
    'female' => array('Gen' => 'сты', 'Dat' => 'сьце', 'Acc' => 'сту', 'Ins' => 'стай', 'Abl' => 'сьце')
  ),
  array(
    'patterns' => array('*зта'),
    'male' => array('Gen' => 'зты', 'Dat' => 'зту', 'Acc' => 'зту', 'Ins' => 'зтам', 'Abl' => 'зту'),
    'female' => array('Gen' => 'зты', 'Dat' => 'зьце', 'Acc' => 'зту', 'Ins' => 'зтай', 'Abl' => 'зьце')
  ),
  array(
    'patterns' => array('*нта'),
    'male' => array('Gen' => 'нты', 'Dat' => 'нту', 'Acc' => 'нту', 'Ins' => 'нтам', 'Abl' => 'нту'),
    'female' => array('Gen' => 'нты', 'Dat' => 'ньце', 'Acc' => 'нту', 'Ins' => 'нтай', 'Abl' => 'нзьце')
  ),
  array(
    'patterns' => array('*лта'),
    'male' => array('Gen' => 'лты', 'Dat' => 'лту', 'Acc' => 'лту', 'Ins' => 'лтам', 'Abl' => 'лту'),
    'female' => array('Gen' => 'лты', 'Dat' => 'льце', 'Acc' => 'лту', 'Ins' => 'лтай', 'Abl' => 'льце')
  ),
  array(
    'patterns' => array('*сда'),
    'male' => array('Gen' => 'сды', 'Dat' => 'сду', 'Acc' => 'сду', 'Ins' => 'сдам', 'Abl' => 'сду'),
    'female' => array('Gen' => 'сды', 'Dat' => 'сьдзе', 'Acc' => 'сду', 'Ins' => 'сдай', 'Abl' => 'сьдзе')
  ),
  array(
    'patterns' => array('*зда'),
    'male' => array('Gen' => 'зды', 'Dat' => 'зду', 'Acc' => 'зду', 'Ins' => 'здам', 'Abl' => 'зду'),
    'female' => array('Gen' => 'зды', 'Dat' => 'зьдзе', 'Acc' => 'зду', 'Ins' => 'здай', 'Abl' => 'зьдзе')
  ),
  array(
    'patterns' => array('*нда'),
    'male' => array('Gen' => 'нды', 'Dat' => 'нду', 'Acc' => 'нду', 'Ins' => 'ндам', 'Abl' => 'нду'),
    'female' => array('Gen' => 'нды', 'Dat' => 'ньдзе', 'Acc' => 'нду', 'Ins' => 'ндай', 'Abl' => 'нзьдзе')
  ),
  array(
    'patterns' => array('*лда'),
    'male' => array('Gen' => 'лды', 'Dat' => 'лду', 'Acc' => 'лду', 'Ins' => 'лдам', 'Abl' => 'лду'),
    'female' => array('Gen' => 'лды', 'Dat' => 'льдзе', 'Acc' => 'лду', 'Ins' => 'лдай', 'Abl' => 'льдзе')
  ),
  array(
    'patterns' => array('*та'),
    'male' => array('Gen' => 'ты', 'Dat' => 'ту', 'Acc' => 'ту', 'Ins' => 'там', 'Abl' => 'ту'),
    'female' => array('Gen' => 'ты', 'Dat' => 'це', 'Acc' => 'ту', 'Ins' => 'тай', 'Abl' => 'це')
  ),
  array(
    'patterns' => array('*да'),
    'male' => array('Gen' => 'ды', 'Dat' => 'ду', 'Acc' => 'ду', 'Ins' => 'дам', 'Abl' => 'ду'),
    'female' => array('Gen' => 'ды', 'Dat' => 'дзе', 'Acc' => 'ду', 'Ins' => 'дай', 'Abl' => 'дзе')
  ),
  array(
    'patterns' => array('*ов(а)', '*ев(а)', '*ав(а)', "*\xb3в(а)"),
    'male' => array('Gen' => 'ы', 'Dat' => 'у', 'Acc' => 'у', 'Ins' => 'ам', 'Abl' => 'у'),
    'female' => array('Gen' => 'ай', 'Dat' => 'ай', 'Acc' => 'у', 'Ins' => 'ай', 'Abl' => 'ай')
  ),
  array(
    'patterns' => array('*а'),
    'male' => array('Gen' => 'ы', 'Dat' => 'у', 'Acc' => 'у', 'Ins' => 'ам', 'Abl' => 'у'),
    'female' => array('Gen' => 'ы', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ай', 'Abl' => 'е')
  ),
  array(
    'patterns' => array('*щ'),
    'male' => array('Gen' => 'шча', 'Dat' => 'шчу', 'Acc' => 'шча', 'Ins' => 'шчам', 'Abl' => 'шчу'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*ов()', '*ев()', '*ав()', "*\xb3в()"),
    'male' => array('Gen' => 'а', 'Dat' => 'у', 'Acc' => 'а', 'Ins' => 'ым', 'Abl' => 'у'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*ок'),
    'male' => array('Gen' => 'ка', 'Dat' => 'ку', 'Acc' => 'ка', 'Ins' => 'ком', 'Abl' => 'ку'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*ак'),
    'male' => array('Gen' => 'ка', 'Dat' => 'ку', 'Acc' => 'ка', 'Ins' => 'кам', 'Abl' => 'ку'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*'),
    'male' => array('Gen' => 'а', 'Dat' => 'у', 'Acc' => 'а', 'Ins' => 'ам', 'Abl' => 'у'),
    'female' => 'fixed'
  ),

);

break;

case 51: // bash

$res['flexible_symbols'] = "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЬЭЮЯабвгдеёжзийклмнопрстуфхцчшщьэюяQWERTYUIOPASDFGHJKLZXCVBNMqwertyuiopasdfghjklzxcvbnm";

$res['names'] = array(
  array(
  'patterns' => array('*а()', '*о()', '*ы()', '*э()', '*е()', '*я()', '*ё()'),
  'male' => array('Gen' => 'ны&#1187;', 'Dat' => '&#1171;а', 'Acc' => 'ны', 'Ins' => 'нан', 'Abl' => 'ла'),
  'female' => array('Gen' => 'ны&#1187;', 'Dat' => '&#1171;а', 'Acc' => 'ны', 'Ins' => 'нан', 'Abl' => 'ла'),
  ),
  array(
  'patterns' => array('*у()', '*р()', '*и()', '*й()', '*ю()'),
  'male' => array('Gen' => '&#1177;ы&#1187;', 'Dat' => '&#1171;а', 'Acc' => '&#1177;ы', 'Ins' => '&#1177;ан', 'Abl' => '&#1177;а'),
  'female' => array('Gen' => '&#1177;ы&#1187;', 'Dat' => '&#1171;а', 'Acc' => '&#1177;ы', 'Ins' => '&#1177;ан', 'Abl' => '&#1177;а'),
  ),
  array(
  'patterns' => array('*р(ь)'),
  'male' => array('Gen' => '&#1177;е&#1187;', 'Dat' => 'г&#1241;', 'Acc' => '&#1177;е', 'Ins' => '&#1177;&#1241;н', 'Abl' => '&#1177;&#1241;'),
  'female' => array('Gen' => '&#1177;е&#1187;', 'Dat' => 'г&#1241;', 'Acc' => '&#1177;е', 'Ins' => '&#1177;&#1241;н', 'Abl' => '&#1177;&#1241;'),
  ),
  array(
  'patterns' => array('*н()', '*л()', '*м()', '*з()', '*ж()'),
  'male' => array('Gen' => 'ды&#1187;', 'Dat' => '&#1171;а', 'Acc' => 'ды', 'Ins' => 'дан', 'Abl' => 'да'),
  'female' => array('Gen' => 'ды&#1187;', 'Dat' => '&#1171;а', 'Acc' => 'ды', 'Ins' => 'дан', 'Abl' => 'да'),
  ),
  array(
  'patterns' => array('*н(ь)', '*л(ь)', '*м(ь)', '*з(ь)'),
  'male' => array('Gen' => 'де&#1187;', 'Dat' => 'г&#1241;', 'Acc' => 'де', 'Ins' => 'д&#1241;н', 'Abl' => 'д&#1241;'),
  'female' => array('Gen' => 'де&#1187;', 'Dat' => 'г&#1241;', 'Acc' => 'де', 'Ins' => 'д&#1241;н', 'Abl' => 'д&#1241;'),
  ),
  array(
  'patterns' => array('*ц()', '*к()', '*г()', '*ш()', '*щ()', '*х()', '*ф()', '*в()', '*п()', '*ч()', '*с()', '*т()', '*б()'),
  'male' => array('Gen' => 'ты&#1187;', 'Dat' => '&#1185;а', 'Acc' => 'ты', 'Ins' => 'тан', 'Abl' => 'та'),
  'female' => array('Gen' => 'ты&#1187;', 'Dat' => '&#1185;а', 'Acc' => 'ты', 'Ins' => 'тан', 'Abl' => 'та'),
  ),
  array(
  'patterns' => array('*к(ь)', '*г(ь)', '*ф(ь)', '*в(ь)', '*п(ь)', '*с(ь)', '*т(ь)', '*б(ь)'),
  'male' => array('Gen' => 'те&#1187;', 'Dat' => 'к&#1241;', 'Acc' => 'те', 'Ins' => 'т&#1241;н', 'Abl' => 'т&#1241;'),
  'female' => array('Gen' => 'те&#1187;', 'Dat' => 'к&#1241;', 'Acc' => 'те', 'Ins' => 'т&#1241;н', 'Abl' => 'т&#1241;'),
  ),
  
  //lat
  
  array(
  'patterns' => array('*a()', '*o()', '*e()'),
  'male' => array('Gen' => '’ны&#1187;', 'Dat' => '’&#1171;а', 'Acc' => '’ны', 'Ins' => '’нан', 'Abl' => '’ла'),
  'female' => array('Gen' => '’ны&#1187;', 'Dat' => '’&#1171;а', 'Acc' => '’ны', 'Ins' => '’нан', 'Abl' => '’ла'),
  ),
  array(
  'patterns' => array('*u()', '*r()', '*i()', '*y()', '*w()'),
  'male' => array('Gen' => '’&#1177;ы&#1187;', 'Dat' => '’&#1171;а', 'Acc' => '’&#1177;ы', 'Ins' => '’&#1177;ан', 'Abl' => '’&#1177;а'),
  'female' => array('Gen' => '’&#1177;ы&#1187;', 'Dat' => '’&#1171;а', 'Acc' => '’&#1177;ы', 'Ins' => '’&#1177;ан', 'Abl' => '’&#1177;а'),
  ),
  array(
  'patterns' => array('*n()', '*l()', '*m()', '*z()', '*j()'),
  'male' => array('Gen' => '’ды&#1187;', 'Dat' => '’&#1171;а', 'Acc' => '’ды', 'Ins' => '’дан', 'Abl' => '’да'),
  'female' => array('Gen' => '’ды&#1187;', 'Dat' => '’&#1171;а', 'Acc' => '’ды', 'Ins' => '’дан', 'Abl' => '’да'),
  ),
  array(
  'patterns' => array('*q()', '*t()', '*p()', '*s()', '*d()', '*f()', '*g()', '*h()', '*k()', '*x()', '*c()', '*v()', '*b()'),
  'male' => array('Gen' => '’ты&#1187;', 'Dat' => '’&#1185;а', 'Acc' => '’ты', 'Ins' => '’тан', 'Abl' => '’та'),
  'female' => array('Gen' => '’ты&#1187;', 'Dat' => '’&#1185;а', 'Acc' => '’ты', 'Ins' => '’тан', 'Abl' => '’та'),
  ),

);

$res['surnames'] = array(
  array(
  'patterns' => array('*а()', '*о()', '*ы()', '*э()', '*е()', '*я()', '*ё()'),
  'male' => array('Gen' => 'ны&#1187;', 'Dat' => '&#1171;а', 'Acc' => 'ны', 'Ins' => 'нан', 'Abl' => 'ла'),
  'female' => array('Gen' => 'ны&#1187;', 'Dat' => '&#1171;а', 'Acc' => 'ны', 'Ins' => 'нан', 'Abl' => 'ла'),
  ),
  array(
  'patterns' => array('*у()', '*р()', '*и()', '*й()', '*ю()'),
  'male' => array('Gen' => '&#1177;ы&#1187;', 'Dat' => '&#1171;а', 'Acc' => '&#1177;ы', 'Ins' => '&#1177;ан', 'Abl' => '&#1177;а'),
  'female' => array('Gen' => '&#1177;ы&#1187;', 'Dat' => '&#1171;а', 'Acc' => '&#1177;ы', 'Ins' => '&#1177;ан', 'Abl' => '&#1177;а'),
  ),
  array(
  'patterns' => array('*р(ь)'),
  'male' => array('Gen' => '&#1177;е&#1187;', 'Dat' => 'г&#1241;', 'Acc' => '&#1177;е', 'Ins' => '&#1177;&#1241;н', 'Abl' => '&#1177;&#1241;'),
  'female' => array('Gen' => '&#1177;е&#1187;', 'Dat' => 'г&#1241;', 'Acc' => '&#1177;е', 'Ins' => '&#1177;&#1241;н', 'Abl' => '&#1177;&#1241;'),
  ),
  array(
  'patterns' => array('*н()', '*л()', '*м()', '*з()', '*ж()'),
  'male' => array('Gen' => 'ды&#1187;', 'Dat' => '&#1171;а', 'Acc' => 'ды', 'Ins' => 'дан', 'Abl' => 'да'),
  'female' => array('Gen' => 'ды&#1187;', 'Dat' => '&#1171;а', 'Acc' => 'ды', 'Ins' => 'дан', 'Abl' => 'да'),
  ),
  array(
  'patterns' => array('*н(ь)', '*л(ь)', '*м(ь)', '*з(ь)'),
  'male' => array('Gen' => 'де&#1187;', 'Dat' => 'г&#1241;', 'Acc' => 'де', 'Ins' => 'д&#1241;н', 'Abl' => 'д&#1241;'),
  'female' => array('Gen' => 'де&#1187;', 'Dat' => 'г&#1241;', 'Acc' => 'де', 'Ins' => 'д&#1241;н', 'Abl' => 'д&#1241;'),
  ),
  array(
  'patterns' => array('*ц()', '*к()', '*г()', '*ш()', '*щ()', '*х()', '*ф()', '*в()', '*п()', '*ч()', '*с()', '*т()', '*б()'),
  'male' => array('Gen' => 'ты&#1187;', 'Dat' => '&#1185;а', 'Acc' => 'ты', 'Ins' => 'тан', 'Abl' => 'та'),
  'female' => array('Gen' => 'ты&#1187;', 'Dat' => '&#1185;а', 'Acc' => 'ты', 'Ins' => 'тан', 'Abl' => 'та'),
  ),
  array(
  'patterns' => array('*к(ь)', '*г(ь)', '*ф(ь)', '*в(ь)', '*п(ь)', '*с(ь)', '*т(ь)', '*б(ь)'),
  'male' => array('Gen' => 'те&#1187;', 'Dat' => 'к&#1241;', 'Acc' => 'те', 'Ins' => 'т&#1241;н', 'Abl' => 'т&#1241;'),
  'female' => array('Gen' => 'те&#1187;', 'Dat' => 'к&#1241;', 'Acc' => 'те', 'Ins' => 'т&#1241;н', 'Abl' => 'т&#1241;'),
  ),
  
  //lat
  
  array(
  'patterns' => array('*a()', '*o()', '*e()'),
  'male' => array('Gen' => '’ны&#1187;', 'Dat' => '’&#1171;а', 'Acc' => '’ны', 'Ins' => '’нан', 'Abl' => '’ла'),
  'female' => array('Gen' => '’ны&#1187;', 'Dat' => '’&#1171;а', 'Acc' => '’ны', 'Ins' => '’нан', 'Abl' => '’ла'),
  ),
  array(
  'patterns' => array('*u()', '*r()', '*i()', '*y()', '*w()'),
  'male' => array('Gen' => '’&#1177;ы&#1187;', 'Dat' => '’&#1171;а', 'Acc' => '’&#1177;ы', 'Ins' => '’&#1177;ан', 'Abl' => '’&#1177;а'),
  'female' => array('Gen' => '’&#1177;ы&#1187;', 'Dat' => '’&#1171;а', 'Acc' => '’&#1177;ы', 'Ins' => '’&#1177;ан', 'Abl' => '’&#1177;а'),
  ),
  array(
  'patterns' => array('*n()', '*l()', '*m()', '*z()', '*j()'),
  'male' => array('Gen' => '’ды&#1187;', 'Dat' => '’&#1171;а', 'Acc' => '’ды', 'Ins' => '’дан', 'Abl' => '’да'),
  'female' => array('Gen' => '’ды&#1187;', 'Dat' => '’&#1171;а', 'Acc' => '’ды', 'Ins' => '’дан', 'Abl' => '’да'),
  ),
  array(
  'patterns' => array('*q()', '*t()', '*p()', '*s()', '*d()', '*f()', '*g()', '*h()', '*k()', '*x()', '*c()', '*v()', '*b()'),
  'male' => array('Gen' => '’ты&#1187;', 'Dat' => '’&#1185;а', 'Acc' => '’ты', 'Ins' => '’тан', 'Abl' => '’та'),
  'female' => array('Gen' => '’ты&#1187;', 'Dat' => '’&#1185;а', 'Acc' => '’ты', 'Ins' => '’тан', 'Abl' => '’та'),
  ),

);

break;

case 100: //imperial

$res['flexible_symbols'] = "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЬЭЮЯIабвгдеёжзийклмнопрстуфхцчшщъьэюяi";
/** /

$res['names'] = array(
  array(
    'patterns' => array('*ь'),
    'male' => array('Gen' => 'я', 'Dat' => 'ю', 'Acc' => 'я', 'Ins' => 'ем', 'Abl' => 'е'),
    'female' => array('Gen' => 'и', 'Dat' => 'и', 'Acc' => 'ь', 'Ins' => 'ью', 'Abl' => 'и')
  ),
  array(
    'patterns' => array('Гюзель', 'Гузель', 'Айгуль', 'Айгюль', 'Гюнэль', 'Гюнель', 'Даниэль', 'Аревикъ',
'Астхикъ', 'Шагикъ', 'Татевикъ', 'Сатеникъ', 'Манушакъ', 'Анушикъ', 'Хасмикъ', 'Назикъ', 'Кайцакъ', 'Ардакъ', 'Арпикъ',
'Гюзяль', 'Жибекъ', 'Асель', 'Николь', 'Аниель', 'Асмикъ', 'Ола', 'Эттель'),
    'male' => 'fixed',
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*о', '*у', '*ы', '*э', '*ё', '*ю', '*и', '*е'),
    'male' => 'fixed',
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*й'),
    'male' => array('Gen' => 'я', 'Dat' => 'ю', 'Acc' => 'я', 'Ins' => 'емъ', 'Abl' => 'е'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*iй'),
    'male' => array('Gen' => 'iя', 'Dat' => 'iю', 'Acc' => 'iя', 'Ins' => 'iемъ', 'Abl' => 'iи'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*я'),
    'male' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'ю', 'Ins' => 'ей', 'Abl' => 'е'),
    'female' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'ю', 'Ins' => 'ей', 'Abl' => 'е'),
  ),
  array(
    'patterns' => array('*iя'),
    'male' => array('Gen' => 'iи', 'Dat' => 'iе', 'Acc' => 'iю', 'Ins' => 'iей', 'Abl' => 'iе'), // почему m и f разные??
    'female' => array('Gen' => 'iи', 'Dat' => 'iи', 'Acc' => 'iю', 'Ins' => 'iей', 'Abl' => 'iи'),
  ),
  array(
    'patterns' => array('Алi(я)', 'Нажi(я)', 'Галi(я)', 'Альфi(я)', 'Балхi(я)', 'Нурi(я)'),
    'male' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'ю', 'Ins' => 'ей', 'Abl' => 'е'),
    'female' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'ю', 'Ins' => 'ей', 'Abl' => 'е'),
  ),
  array(
    'patterns' => array('*а'),
    'male' => array('Gen' => 'ы', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'),
    'female' => array('Gen' => 'ы', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'),
  ),
  array(
    'patterns' => array('*к(а)', '*х(а)', '*г(а)'),
    'male' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'),
    'female' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'),
  ),
  array(
    'patterns' => array('*ш(а)', '*ж(а)', '*ч(а)', '*щ(а)'),
    'male' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ей', 'Abl' => 'е'),
    'female' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ей', 'Abl' => 'е'),
  ),
  array(
    'patterns' => array('*ца'),
    'male' => array('Gen' => 'цы', 'Dat' => 'це', 'Acc' => 'цу', 'Ins' => 'цей', 'Abl' => 'це'),
    'female' => array('Gen' => 'цы', 'Dat' => 'це', 'Acc' => 'цу', 'Ins' => 'цей', 'Abl' => 'це'),
  ),
  array(
    'patterns' => array('Миляуш(а)'),
    'male' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'),
    'female' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'),
  ),
  array(
    'patterns' => array('*', '*(ъ)'),
    'male' => array('Gen' => 'а', 'Dat' => 'у', 'Acc' => 'а', 'Ins' => 'омъ', 'Abl' => 'е'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Пётръ'),
    'male' => array('Gen' => 'Петра', 'Dat' => 'Петру', 'Acc' => 'Петра', 'Ins' => 'Петромъ', 'Abl' => 'Петре'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Павелъ'),
    'male' => array('Gen' => 'Павла', 'Dat' => 'Павлу', 'Acc' => 'Павла', 'Ins' => 'Павломъ', 'Abl' => 'Павле'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Левъ'),
    'male' => array('Gen' => 'Льва', 'Dat' => 'Льву', 'Acc' => 'Льва', 'Ins' => 'Львомъ', 'Abl' => 'Льве'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('*а(ёкъ)', '*о(ёкъ)', '*у(ёкъ)', '*е(ёкъ)', '*и(ёкъ)'),
    'male' => array('Gen' => 'йка', 'Dat' => 'йку', 'Acc' => 'йка', 'Ins' => 'йкомъ', 'Abl' => 'йке'),
    'female' => array('Gen' => 'йка', 'Dat' => 'йку', 'Acc' => 'йка', 'Ins' => 'йкомъ', 'Abl' => 'йке'),
  ),
  array(
    'patterns' => array('*ёкъ'),
    'male' => array('Gen' => 'ька', 'Dat' => 'ьку', 'Acc' => 'ька', 'Ins' => 'ькомъ', 'Abl' => 'ьке'),
    'female' => array('Gen' => 'ька', 'Dat' => 'ьку', 'Acc' => 'ька', 'Ins' => 'ькомъ', 'Abl' => 'ьке'),
  ),
  array(
    'patterns' => array('*окъ'),
    'male' => array('Gen' => 'ка', 'Dat' => 'ку', 'Acc' => 'ка', 'Ins' => 'комъ', 'Abl' => 'ке'),
    'female' => array('Gen' => 'ка', 'Dat' => 'ку', 'Acc' => 'ка', 'Ins' => 'комъ', 'Abl' => 'ке'),
  ),
  array(
    'patterns' => array('Юрецъ'),
    'male' => array('Gen' => 'Юрца', 'Dat' => 'Юрцу', 'Acc' => 'Юрца', 'Ins' => 'Юрцомъ', 'Abl' => 'Юрце'),
    'female' => 'fixed',
  ),
);

$res['surnames'] = array(
  array(
    'patterns' => array('*о', '*у', '*ы', '*э', '*ё', '*ю', '*и', '*е'),
    'male' => 'fixed',
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*я'),
    'male' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'ю', 'Ins' => 'ей', 'Abl' => 'е'),
    'female' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'ю', 'Ins' => 'ей', 'Abl' => 'е'),
  ),
  array(
    'patterns' => array('Цхака(я)', 'Баркла(я)', 'Арча(я)', 'Сана(я)', 'Бера(я)'),
    'male' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'ю', 'Ins' => 'ей', 'Abl' => 'е'),
    'female' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'ю', 'Ins' => 'ей', 'Abl' => 'е'),
  ),
  array(
    'patterns' => array('*ая'),
    'male' => array('Gen' => 'аи', 'Dat' => 'ае', 'Acc' => 'аю', 'Ins' => 'аей', 'Abl' => 'ае'),
    'female' => array('Gen' => 'ой', 'Dat' => 'ой', 'Acc' => 'ую', 'Ins' => 'ой', 'Abl' => 'ой'),
  ),
  array(
    'patterns' => array('*ж(ая)', '*ш(ая)', '*щ(ая)', '*ч(ая)', '*ц(ая)'),
    'male' => array('Gen' => 'аи', 'Dat' => 'ае', 'Acc' => 'аю', 'Ins' => 'аей', 'Abl' => 'ае'),
    'female' => array('Gen' => 'ей', 'Dat' => 'ей', 'Acc' => 'ую', 'Ins' => 'ей', 'Abl' => 'ей'),
  ),
  array(
    'patterns' => array('*яя'),
    'male' => 'fixed',
    'female' => array('Gen' => 'ей', 'Dat' => 'ей', 'Acc' => 'юю', 'Ins' => 'ей', 'Abl' => 'ей'),
  ),
  
  array(
    'patterns' => array('*iя'),
    'male' => array('Gen' => 'iи', 'Dat' => 'iи', 'Acc' => 'iю', 'Ins' => 'iей', 'Abl' => 'iи'), // почему m и f разные??
    'female' => array('Gen' => 'iи', 'Dat' => 'iе', 'Acc' => 'iю', 'Ins' => 'iей', 'Abl' => 'iе'),
  ),
  array(
    'patterns' => array('Мякеля', 'Лямся', 'Талья', 'Луя', 'Рейня', 'Ростобая', 'Пелля', 'Время', 'Титма'),
    'male' => 'fixed',
    'female' => 'fixed', 
  ),
  array(
    'patterns' => array('*а'),
    'male' => array('Gen' => 'ы', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'),
    'female' => array('Gen' => 'ы', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'), 
  ),
  array(
    'patterns' => array('*ов(а)', '*ев(а)', '*ёв(а)', '*ын(а)', '*ин(а)'),
    'male' => array('Gen' => 'ы', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'),
    'female' => array('Gen' => 'ой', 'Dat' => 'ой', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'ой'), 
  ),
  array(
    'patterns' => array('Былин(а)'),
    'male' => array('Gen' => 'ы', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'),
    'female' => array('Gen' => 'ы', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'), 
  ),
  array(
    'patterns' => array('Швидк(а)'),
    'male' => array('Gen' => 'а', 'Dat' => 'у', 'Acc' => 'у', 'Ins' => 'омъ', 'Abl' => 'е'),
    'female' => array('Gen' => 'ой', 'Dat' => 'ой', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'ой'), 
  ),
  array(
    'patterns' => array('Ирчишен(а)'),
    'male' => array('Gen' => 'ы', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'),
    'female' => array('Gen' => 'ой', 'Dat' => 'ой', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'ой'), 
  ),
  array(
    'patterns' => array('Бов(а)', 'Сов(а)', 'Худын(а)', 'Щербин(а)', 'Калин(а)'),
    'male' => array('Gen' => 'ы', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'),
    'female' => array('Gen' => 'ы', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'), 
  ),
  
  array(
    'patterns' => array('*у(а)', '*и(а)', '*э(а)', '*е(а)', '*ю(а)', '*а(а)', '*о(а)'),
    'male' => 'fixed',
    'female' => 'fixed', 
  ),
  array(
    'patterns' => array('*к(а)', '*х(а)', '*г(а)'),
    'male' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'),
    'female' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'), 
  ),
  array(
    'patterns' => array('*ш(а)', '*ж(а)', '*ч(а)', '*щ(а)'),
    'male' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ей', 'Abl' => 'е'),
    'female' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ей', 'Abl' => 'е'), 
  ),
  array(
    'patterns' => array('Кваш(а)', 'Ганж(а)', 'Уш(а)', 'Ханаш(а)'),
    'male' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'),
    'female' => array('Gen' => 'и', 'Dat' => 'е', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'е'), 
  ),
  array(
    'patterns' => array('*ца'),
    'male' => array('Gen' => 'цы', 'Dat' => 'це', 'Acc' => 'цу', 'Ins' => 'цей', 'Abl' => 'це'),
    'female' => array('Gen' => 'цы', 'Dat' => 'це', 'Acc' => 'цу', 'Ins' => 'цей', 'Abl' => 'це'), 
  ),
  array(
    'patterns' => array('Дюм(а)', 'Петип(а)', 'Кешелав(а)', 'Малек(а)', 'Рошк(а)', 'Ракш(а)', 'Гар(а)', 'Шкут(а)', 
      'Опар(а)', 'Ф(а)', 'Бег(а)', 'Туг(а)', 'Дюб(а)'),
    'male' => 'fixed',
    'female' => 'fixed' 
  ),
  array(
    'patterns' => array('*ь'),
    'male' => array('Gen' => 'я', 'Dat' => 'ю', 'Acc' => 'я', 'Ins' => 'емъ', 'Abl' => 'е'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*ень', 'Уласе(нь)', 'Пиве(нь)', 'Се(нь)', 'Ме(нь)', 'Хебе(нь)', 'Ште(нь)', 'Дере(нь)', 'Липе(нь)'), // any "*ень" like "Уласень"? // Alexey: except monosyllabic, I guess
    'male' => array('Gen' => 'ня', 'Dat' => 'ню', 'Acc' => 'ня', 'Ins' => 'немъ', 'Abl' => 'не'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('Журав(ель)'),
    'male' => array('Gen' => 'ля', 'Dat' => 'лю', 'Acc' => 'ля', 'Ins' => 'лемъ', 'Abl' => 'ле'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*й', 'Берего(й)', 'Водосто(й)', 'Корро(й)', 'Коро(й)', 'Геро(й)', 'Стро(й)', 'Алло(й)', 'Градобо(й)'),
    'male' => array('Gen' => 'я', 'Dat' => 'ю', 'Acc' => 'я', 'Ins' => 'емъ', 'Abl' => 'е'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('Бо(й)', 'Во(й)', 'Го(й)', 'До(й)', 'Ко(й)', 'Ло(й)', 'Но(й)', 'Ро(й)', 'Со(й)', 'То(й)', 'Ко(й)', 'Ло(й)', 'Уо(й)', 'Фо(й)', 'Хо(й)', 'Цо(й)', 'Чо(й)', 'Шо(й)'),
    'male' => array('Gen' => 'я', 'Dat' => 'ю', 'Acc' => 'я', 'Ins' => 'емъ', 'Abl' => 'е'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*ой', '*ый'),
    'male' => array('Gen' => 'ого', 'Dat' => 'ому', 'Acc' => 'ого', 'Ins' => 'ымъ', 'Abl' => 'омъ'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*цый'),
    'male' => array('Gen' => 'цего', 'Dat' => 'цему', 'Acc' => 'цего', 'Ins' => 'цымъ', 'Abl' => 'цемъ'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*г(ой)', '*к(ой)', '*х(ой)', '*ж(ой)', '*ш(ой)', '*щ(ой)', '*ч(ой)'),
    'male' => array('Gen' => 'ого', 'Dat' => 'ому', 'Acc' => 'ого', 'Ins' => 'имъ', 'Abl' => 'омъ'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*iй'),
    'male' => array('Gen' => 'iя', 'Dat' => 'iю', 'Acc' => 'iя', 'Ins' => 'iемъ', 'Abl' => 'iи'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*г(iй)', '*к(iй)', '*х(iй)'),
    'male' => array('Gen' => 'ого', 'Dat' => 'ому', 'Acc' => 'ого', 'Ins' => 'имъ', 'Abl' => 'омъ'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*ч(iй)', '*ж(iй)', '*ш(iй)', '*щ(iй)', '*н(iй)'),
    'male' => array('Gen' => 'его', 'Dat' => 'ему', 'Acc' => 'его', 'Ins' => 'имъ', 'Abl' => 'емъ'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('Арсенi(й)', 'Гуржi(й)', 'Трухнi(й)', 'Мохi(й)', 'Топчi(й)', 'Багрi(й)', 'Тульчi(й)', 'Саланжi(й)'),
    'male' => array('Gen' => 'я', 'Dat' => 'ю', 'Acc' => 'я', 'Ins' => 'емъ', 'Abl' => 'и'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*ей'),
    'male' => array('Gen' => 'ея', 'Dat' => 'ею', 'Acc' => 'ея', 'Ins' => 'еемъ', 'Abl' => 'ее'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('Солов(ей)', 'Вороб(ей)'),
    'male' => array('Gen' => 'ья', 'Dat' => 'ью', 'Acc' => 'ья', 'Ins' => 'ьемъ', 'Abl' => 'ье'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('Кротъ', 'Сукъ', 'Гнепъ', 'Кремсъ', 'Чанъ', 'Фа', 'Кустъ'),
    'male' => 'fixed',
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('*ихъ', '*ыхъ'),
    'male' => 'fixed',
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('*окъ', '*екъ', '*ч(ёкъ)'),
    'male' => array('Gen' => 'ка', 'Dat' => 'ку', 'Acc' => 'ка', 'Ins' => 'комъ', 'Abl' => 'ке'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Войтише(къ)', 'Цве(къ)', 'Дуде(къ)', 'Буче(къ)', 'Ляше(къ)', 'Ше(къ)', 'Клаче(къ)', 'Баче(къ)',
      'Корчмаре(къ)', 'Дяче(къ)', 'Цумбе(къ)', 'Собе-Пане(къ)', 'Штре(къ)', 'Сире(къ)', 'Псуе(къ)', 'Матие(къ)', 'Пане(къ)', 
      'Поле(къ)', 'Саде(къ)', 'Стро(къ)', 'Ско(къ)', 'Смо(къ)', 'Воло(къ)', 'Набо(къ)', 'Бро(къ)', 'Шейнро(къ)', 'Скаче(къ)', 'Гае(къ)'),
    'male' => array('Gen' => 'ка', 'Dat' => 'ку', 'Acc' => 'ка', 'Ins' => 'комъ', 'Abl' => 'ке'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('*ёкъ'),
    'male' => array('Gen' => 'ька', 'Dat' => 'ьку', 'Acc' => 'ька', 'Ins' => 'ькомъ', 'Abl' => 'ьке'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('*ецъ'),
    'male' => array('Gen' => 'ца', 'Dat' => 'цу', 'Acc' => 'ца', 'Ins' => 'цемъ', 'Abl' => 'це'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Жуклин(ецъ)', 'Гапан(ецъ)', 'Кремен(ецъ)', 'Ворон(ецъ)',
      'Рухов(ецъ)', 'От(ецъ)', 'Чигрин(ецъ)', 'Тестел(ецъ)', 'Короб(ецъ)', 'Лубен(ецъ)', 'Крав(ецъ)', 'Шв(ецъ)', 'Жн(ецъ)',
      'Бо(ецъ)', 'Титов(ецъ)', 'Скреб(ецъ)', 'Канив(ецъ)', 'Митьков(ецъ)', 'Зимов(ецъ)', 'Мул(ецъ)', 'Дон(ецъ)', 'Сидор(ецъ)', 'Туров(ецъ)',
      'Ливин(ецъ)', 'Чуднов(ецъ)', 'Мыслив(ецъ)', 'Бел(ецъ)', 'Малахов(ецъ)', 'Козуб(ецъ)', 'Казан(ецъ)', 'Якуб(ецъ)', 'Козин(ецъ)', 'Москал(ецъ)',
      'Шабан(ецъ)', 'Корни(ецъ)', 'Степан(ецъ)', 'Брагин(ецъ)', 'Левин(ецъ)', 'Руб(ецъ)', 'Кацав(ецъ)', 'Остап(ецъ)'),
    'male' => array('Gen' => 'ца', 'Dat' => 'цу', 'Acc' => 'ца', 'Ins' => 'цомъ', 'Abl' => 'це'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Е(лецъ)'),
    'male' => array('Gen' => 'льца', 'Dat' => 'льцу', 'Acc' => 'льца', 'Ins' => 'льцомъ', 'Abl' => 'льце'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('*лецъ'),
    'male' => array('Gen' => 'льца', 'Dat' => 'льцу', 'Acc' => 'льца', 'Ins' => 'льцемъ', 'Abl' => 'льце'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('*а(ецъ)', '*е(ецъ)', '*и(ецъ)', '*о(ецъ)', '*у(ецъ)', '*ю(ецъ)', '*я(ецъ)', '*ы(ецъ)'),
    'male' => array('Gen' => 'йца', 'Dat' => 'йцу', 'Acc' => 'йца', 'Ins' => 'йцемъ', 'Abl' => 'йце'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Шв(ецъ)', 'Жн(ецъ)', 'За(ецъ)', 'Б(ецъ)', 'Бразн(ецъ)', 'Гр(ецъ)', 'Ойн(ецъ)', 'Хейф(ецъ)'),
    'male' => array('Gen' => 'еца', 'Dat' => 'ецу', 'Acc' => 'еца', 'Ins' => 'ецомъ', 'Abl' => 'еце'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Кеб(ецъ)', 'Ге(ецъ)'),
    'male' => array('Gen' => 'еца', 'Dat' => 'ецу', 'Acc' => 'еца', 'Ins' => 'ецемъ', 'Abl' => 'еце'), // Ins не такой, как в предыдущем паттерне
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Вет(еръ)'),
    'male' => array('Gen' => 'ра', 'Dat' => 'ру', 'Acc' => 'ра', 'Ins' => 'ромъ', 'Abl' => 'ре'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('*ёлъ'),
    'male' => array('Gen' => 'ла', 'Dat' => 'лу', 'Acc' => 'ла', 'Ins' => 'ломъ', 'Abl' => 'ле'),
    'female' => 'fixed',
  ),
  array(
    'patterns' => array('Каптёлъ'),
    'male' => 'fixed',
    'female' => 'fixed',
  ),
  
  array(
    'patterns' => array('*', '*(ъ)'),
    'male' => array('Gen' => 'а', 'Dat' => 'у', 'Acc' => 'а', 'Ins' => 'омъ', 'Abl' => 'е'),
    'female' => 'fixed',
  ),
  
  array(
    'patterns' => array('Варкентин(ъ)', 'Эллин(ъ)'),
    'male' => array('Gen' => 'а', 'Dat' => 'у', 'Acc' => 'а', 'Ins' => 'омъ', 'Abl' => 'е'),
    'female' => 'fixed',
  ),

  array(
    'patterns' => array('*ев(ъ)', '*ов(ъ)', '*ёв(ъ)', '*ин(ъ)', '*ын(ъ)', 'Ковалышен(ъ)'), // *ув?
    'male' => array('Gen' => 'а', 'Dat' => 'у', 'Acc' => 'а', 'Ins' => 'ымъ', 'Abl' => 'е'),
    'female' => 'fixed',
  ),

  array(
    'patterns' => array('*ч(ъ)', '*ш(ъ)', '*ц(ъ)', '*щ(ъ)', '*ж(ъ)'),
    'male' => array('Gen' => 'а', 'Dat' => 'у', 'Acc' => 'а', 'Ins' => 'емъ', 'Abl' => 'е'),
    'female' => 'fixed',
  ),

  array(
    'patterns' => array('Сыч(ъ)', 'Карандаш(ъ)', 'Чиж(ъ)', 'Кулиш(ъ)', 'Пыж(ъ)', 'Барабаш(ъ)', 'Пархач(ъ)', 'Деркач(ъ)', 'Грин(ъ)', 'Шин(ъ)'),
    'male' => array('Gen' => 'а', 'Dat' => 'у', 'Acc' => 'а', 'Ins' => 'омъ', 'Abl' => 'е'),
    'female' => 'fixed',
  ),
  
  array(
    'patterns' => array("Тер-", "Нор-", "Сулима-", "Бей-", "Джан-", "Гаген-"),
    'male' => 'fixed',
    'female' => 'fixed',
  ),
 
);

/**/

$res['names'] = array(
 array(
   'patterns' => array('*ь'),
   'male' => array('Gen' => 'я', 'Dat' => 'ю', 'Acc' => 'я', 'Ins' =>
'ем', 'Abl' => '&#1123;'),
   'female' => array('Gen' => 'и', 'Dat' => 'и', 'Acc' => 'ь', 'Ins'
=> 'ью', 'Abl' => 'и')
 ),
 array(
   'patterns' => array('Гюзель', 'Гузель', 'Айгуль', 'Айгюль',
'Гюнэль', 'Гюнель', 'Даниэль', 'Аревикъ',
'Астхикъ', 'Шагикъ', 'Татевикъ', 'Сатеникъ', 'Манушакъ', 'Анушикъ',
'Хасмикъ', 'Назикъ', 'Кайцакъ', 'Ардакъ', 'Арпикъ',
'Гюзяль', 'Жибекъ', 'Асель', 'Николь', 'Аниель', 'Асмикъ', 'Ола', 'Эттель'),
   'male' => 'fixed',
   'female' => 'fixed'
 ),
 array(
   'patterns' => array('*о', '*у', '*ы', '*э', '*ё', '*ю', '*и', '*е'),
   'male' => 'fixed',
   'female' => 'fixed'
 ),
 array(
   'patterns' => array('*й'),
   'male' => array('Gen' => 'я', 'Dat' => 'ю', 'Acc' => 'я', 'Ins' =>
'емъ', 'Abl' => '&#1123;'),
   'female' => 'fixed'
 ),
 array(
   'patterns' => array('*iй'),
   'male' => array('Gen' => 'iя', 'Dat' => 'iю', 'Acc' => 'iя', 'Ins'
=> 'iемъ', 'Abl' => 'iи'),
   'female' => 'fixed'
 ),
 array(
   'patterns' => array('*я'),
   'male' => array('Gen' => 'и', 'Dat' => '&#1123;', 'Acc' => 'ю', 'Ins' =>
'ей', 'Abl' => '&#1123;'),
   'female' => array('Gen' => 'и', 'Dat' => '&#1123;', 'Acc' => 'ю', 'Ins'
=> 'ей', 'Abl' => '&#1123;'),
 ),
 array(
   'patterns' => array('*iя'),
   'male' => array('Gen' => 'iи', 'Dat' => 'i&#1123;', 'Acc' => 'iю', 'Ins'
=> 'iей', 'Abl' => 'i&#1123;'), // почему m и f разные??
   'female' => array('Gen' => 'iи', 'Dat' => 'iи', 'Acc' => 'iю',
'Ins' => 'iей', 'Abl' => 'iи'),
 ),
 array(
   'patterns' => array('Алi(я)', 'Нажi(я)', 'Галi(я)', 'Альфi(я)',
'Балхi(я)', 'Нурi(я)'),
   'male' => array('Gen' => 'и', 'Dat' => '&#1123;', 'Acc' => 'ю', 'Ins' =>
'ей', 'Abl' => '&#1123;'),
   'female' => array('Gen' => 'и', 'Dat' => '&#1123;', 'Acc' => 'ю', 'Ins'
=> 'ей', 'Abl' => '&#1123;'),
 ),
 array(
   'patterns' => array('*а'),
   'male' => array('Gen' => 'ы', 'Dat' => '&#1123;', 'Acc' => 'у', 'Ins' =>
'ой', 'Abl' => '&#1123;'),
   'female' => array('Gen' => 'ы', 'Dat' => '&#1123;', 'Acc' => 'у', 'Ins'
=> 'ой', 'Abl' => '&#1123;'),
 ),
 array(
   'patterns' => array('*к(а)', '*х(а)', '*г(а)'),
   'male' => array('Gen' => 'и', 'Dat' => '&#1123;', 'Acc' => 'у', 'Ins' =>
'ой', 'Abl' => '&#1123;'),
   'female' => array('Gen' => 'и', 'Dat' => '&#1123;', 'Acc' => 'у', 'Ins'
=> 'ой', 'Abl' => '&#1123;'),
 ),
 array(
   'patterns' => array('*ш(а)', '*ж(а)', '*ч(а)', '*щ(а)'),
   'male' => array('Gen' => 'и', 'Dat' => '&#1123;', 'Acc' => 'у', 'Ins' =>
'ей', 'Abl' => '&#1123;'),
   'female' => array('Gen' => 'и', 'Dat' => '&#1123;', 'Acc' => 'у', 'Ins'
=> 'ей', 'Abl' => '&#1123;'),
 ),
 array(
   'patterns' => array('*ца'),
   'male' => array('Gen' => 'цы', 'Dat' => 'ц&#1123;', 'Acc' => 'цу', 'Ins'
=> 'цей', 'Abl' => 'ц&#1123;'),
   'female' => array('Gen' => 'цы', 'Dat' => 'ц&#1123;', 'Acc' => 'цу',
'Ins' => 'цей', 'Abl' => 'ц&#1123;'),
 ),
 array(
   'patterns' => array('Миляуш(а)'),
   'male' => array('Gen' => 'и', 'Dat' => '&#1123;', 'Acc' => 'у', 'Ins' =>
'ой', 'Abl' => '&#1123;'),
   'female' => array('Gen' => 'и', 'Dat' => '&#1123;', 'Acc' => 'у', 'Ins'
=> 'ой', 'Abl' => '&#1123;'),
 ),
 array(
   'patterns' => array('*', '*(ъ)'),
   'male' => array('Gen' => 'а', 'Dat' => 'у', 'Acc' => 'а', 'Ins' =>
'омъ', 'Abl' => '&#1123;'),
   'female' => 'fixed',
 ),
 array(
   'patterns' => array('Пётръ'),
   'male' => array('Gen' => 'Петра', 'Dat' => 'Петру', 'Acc' =>
'Петра', 'Ins' => 'Петромъ', 'Abl' => 'Петр&#1123;'),
   'female' => 'fixed',
 ),
 array(
   'patterns' => array('Павелъ'),
   'male' => array('Gen' => 'Павла', 'Dat' => 'Павлу', 'Acc' =>
'Павла', 'Ins' => 'Павломъ', 'Abl' => 'Павл&#1123;'),
   'female' => 'fixed',
 ),
 array(
   'patterns' => array('Левъ'),
   'male' => array('Gen' => 'Льва', 'Dat' => 'Льву', 'Acc' => 'Льва',
'Ins' => 'Львомъ', 'Abl' => 'Льв&#1123;'),
   'female' => 'fixed',
 ),
 array(
   'patterns' => array('*а(ёкъ)', '*о(ёкъ)', '*у(ёкъ)', '*е(ёкъ)', '*и(ёкъ)'),
   'male' => array('Gen' => 'йка', 'Dat' => 'йку', 'Acc' => 'йка',
'Ins' => 'йкомъ', 'Abl' => 'йк&#1123;'),
   'female' => array('Gen' => 'йка', 'Dat' => 'йку', 'Acc' => 'йка',
'Ins' => 'йкомъ', 'Abl' => 'йк&#1123;'),
 ),
 array(
   'patterns' => array('*ёкъ'),
   'male' => array('Gen' => 'ька', 'Dat' => 'ьку', 'Acc' => 'ька',
'Ins' => 'ькомъ', 'Abl' => 'ьк&#1123;'),
   'female' => array('Gen' => 'ька', 'Dat' => 'ьку', 'Acc' => 'ька',
'Ins' => 'ькомъ', 'Abl' => 'ьк&#1123;'),
 ),
 array(
   'patterns' => array('*окъ'),
   'male' => array('Gen' => 'ка', 'Dat' => 'ку', 'Acc' => 'ка', 'Ins'
=> 'комъ', 'Abl' => 'к&#1123;'),
   'female' => array('Gen' => 'ка', 'Dat' => 'ку', 'Acc' => 'ка',
'Ins' => 'комъ', 'Abl' => 'к&#1123;'),
 ),
 array(
   'patterns' => array('Юрецъ'),
   'male' => array('Gen' => 'Юрца', 'Dat' => 'Юрцу', 'Acc' => 'Юрца',
'Ins' => 'Юрцомъ', 'Abl' => 'Юрц&#1123;'),
   'female' => 'fixed',
 ),
);

$res['surnames'] = array(
 array(
   'patterns' => array('*о', '*у', '*ы', '*э', '*ё', '*ю', '*и', '*е'),
   'male' => 'fixed',
   'female' => 'fixed'
 ),
 array(
   'patterns' => array('*я'),
   'male' => array('Gen' => 'и', 'Dat' => '&#1123;', 'Acc' => 'ю', 'Ins' =>
'ей', 'Abl' => '&#1123;'),
   'female' => array('Gen' => 'и', 'Dat' => '&#1123;', 'Acc' => 'ю', 'Ins'
=> 'ей', 'Abl' => '&#1123;'),
 ),
 array(
   'patterns' => array('Цхака(я)', 'Баркла(я)', 'Арча(я)', 'Сана(я)',
'Бера(я)'),
   'male' => array('Gen' => 'и', 'Dat' => '&#1123;', 'Acc' => 'ю', 'Ins' =>
'ей', 'Abl' => '&#1123;'),
   'female' => array('Gen' => 'и', 'Dat' => '&#1123;', 'Acc' => 'ю', 'Ins'
=> 'ей', 'Abl' => '&#1123;'),
 ),
 array(
   'patterns' => array('*ая'),
   'male' => array('Gen' => 'аи', 'Dat' => 'а&#1123;', 'Acc' => 'аю', 'Ins'
=> 'аей', 'Abl' => 'а&#1123;'),
   'female' => array('Gen' => 'ой', 'Dat' => 'ой', 'Acc' => 'ую',
'Ins' => 'ой', 'Abl' => 'ой'),
 ),
 array(
   'patterns' => array('*ж(ая)', '*ш(ая)', '*щ(ая)', '*ч(ая)', '*ц(ая)'),
   'male' => array('Gen' => 'аи', 'Dat' => 'а&#1123;', 'Acc' => 'аю', 'Ins'
=> 'аей', 'Abl' => 'а&#1123;'),
   'female' => array('Gen' => 'ей', 'Dat' => 'ей', 'Acc' => 'ую',
'Ins' => 'ей', 'Abl' => 'ей'),
 ),
 array(
   'patterns' => array('*яя'),
   'male' => 'fixed',
   'female' => array('Gen' => 'ей', 'Dat' => 'ей', 'Acc' => 'юю',
'Ins' => 'ей', 'Abl' => 'ей'),
 ),

 array(
   'patterns' => array('*iя'),
   'male' => array('Gen' => 'iи', 'Dat' => 'iи', 'Acc' => 'iю', 'Ins'
=> 'iей', 'Abl' => 'iи'), // почему m и f разные??
   'female' => array('Gen' => 'iи', 'Dat' => 'i&#1123;', 'Acc' => 'iю',
'Ins' => 'iей', 'Abl' => 'i&#1123;'),
 ),
 array(
   'patterns' => array('Мякеля', 'Лямся', 'Талья', 'Луя', 'Рейня',
'Ростобая', 'Пелля', 'Время', 'Титма'),
   'male' => 'fixed',
   'female' => 'fixed',
 ),
 array(
   'patterns' => array('*а'),
   'male' => array('Gen' => 'ы', 'Dat' => '&#1123;', 'Acc' => 'у', 'Ins' =>
'ой', 'Abl' => '&#1123;'),
   'female' => array('Gen' => 'ы', 'Dat' => '&#1123;', 'Acc' => 'у', 'Ins'
=> 'ой', 'Abl' => '&#1123;'),
 ),
 array(
   'patterns' => array('*ов(а)', '*ев(а)', '*ёв(а)', '*ын(а)', '*ин(а)'),
   'male' => array('Gen' => 'ы', 'Dat' => '&#1123;', 'Acc' => 'у', 'Ins' =>
'ой', 'Abl' => '&#1123;'),
   'female' => array('Gen' => 'ой', 'Dat' => 'ой', 'Acc' => 'у',
'Ins' => 'ой', 'Abl' => 'ой'),
 ),
 array(
   'patterns' => array('Былин(а)'),
   'male' => array('Gen' => 'ы', 'Dat' => '&#1123;', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => '&#1123;'),
   'female' => array('Gen' => 'ы', 'Dat' => '&#1123;', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => '&#1123;'),
 ),
 array(
   'patterns' => array('Швидк(а)'),
   'male' => array('Gen' => 'а', 'Dat' => 'у', 'Acc' => 'у', 'Ins' => 'омъ', 'Abl' => '&#1123;'),
   'female' => array('Gen' => 'ой', 'Dat' => 'ой', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'ой'),
 ),
 array(
   'patterns' => array('Ирчишен(а)'),
   'male' => array('Gen' => 'ы', 'Dat' => '&#1123;', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => '&#1123;'),
   'female' => array('Gen' => 'ой', 'Dat' => 'ой', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => 'ой'),
 ),
 array(
   'patterns' => array('Бов(а)', 'Сов(а)', 'Худын(а)', 'Щербин(а)',
'Калин(а)'),
   'male' => array('Gen' => 'ы', 'Dat' => '&#1123;', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => '&#1123;'),
   'female' => array('Gen' => 'ы', 'Dat' => '&#1123;', 'Acc' => 'у', 'Ins' => 'ой', 'Abl' => '&#1123;'),
 ),

 array(
   'patterns' => array('*у(а)', '*и(а)', '*э(а)', '*е(а)', '*ю(а)', '*а(а)', '*о(а)'),
   'male' => 'fixed',
   'female' => 'fixed',
 ),
 array(
   'patterns' => array('*к(а)', '*х(а)', '*г(а)'),
   'male' => array('Gen' => 'и', 'Dat' => '&#1123;', 'Acc' => 'у', 'Ins' =>
'ой', 'Abl' => '&#1123;'),
   'female' => array('Gen' => 'и', 'Dat' => '&#1123;', 'Acc' => 'у', 'Ins'
=> 'ой', 'Abl' => '&#1123;'),
 ),
 array(
   'patterns' => array('*ш(а)', '*ж(а)', '*ч(а)', '*щ(а)'),
   'male' => array('Gen' => 'и', 'Dat' => '&#1123;', 'Acc' => 'у', 'Ins' =>
'ей', 'Abl' => '&#1123;'),
   'female' => array('Gen' => 'и', 'Dat' => '&#1123;', 'Acc' => 'у', 'Ins'
=> 'ей', 'Abl' => '&#1123;'),
 ),
 array(
   'patterns' => array('Кваш(а)', 'Ганж(а)', 'Уш(а)', 'Ханаш(а)'),
   'male' => array('Gen' => 'и', 'Dat' => '&#1123;', 'Acc' => 'у', 'Ins' =>
'ой', 'Abl' => '&#1123;'),
   'female' => array('Gen' => 'и', 'Dat' => '&#1123;', 'Acc' => 'у', 'Ins'
=> 'ой', 'Abl' => '&#1123;'),
 ),
 array(
   'patterns' => array('*ца'),
   'male' => array('Gen' => 'цы', 'Dat' => 'ц&#1123;', 'Acc' => 'цу', 'Ins'
=> 'цей', 'Abl' => 'ц&#1123;'),
   'female' => array('Gen' => 'цы', 'Dat' => 'ц&#1123;', 'Acc' => 'цу',
'Ins' => 'цей', 'Abl' => 'ц&#1123;'),
 ),
 array(
   'patterns' => array('Дюм(а)', 'Петип(а)', 'Кешелав(а)',
'Малек(а)', 'Рошк(а)', 'Ракш(а)', 'Гар(а)', 'Шкут(а)',
     'Опар(а)', 'Ф(а)', 'Бег(а)', 'Туг(а)', 'Дюб(а)'),
   'male' => 'fixed',
   'female' => 'fixed'
 ),
 array(
   'patterns' => array('*ь'),
   'male' => array('Gen' => 'я', 'Dat' => 'ю', 'Acc' => 'я', 'Ins' =>
'емъ', 'Abl' => '&#1123;'),
   'female' => 'fixed'
 ),
 array(
   'patterns' => array('*ень', 'Уласе(нь)', 'Пиве(нь)', 'Се(нь)',
'Ме(нь)', 'Хебе(нь)', 'Ште(нь)', 'Дере(нь)', 'Липе(нь)'), // any "*ень" like "Уласень"? // Alexey: except monosyllabic, I guess
   'male' => array('Gen' => 'ня', 'Dat' => 'ню', 'Acc' => 'ня', 'Ins'
=> 'немъ', 'Abl' => 'н&#1123;'),
   'female' => 'fixed'
 ),
 array(
   'patterns' => array('Журав(ель)'),
   'male' => array('Gen' => 'ля', 'Dat' => 'лю', 'Acc' => 'ля', 'Ins'
=> 'лемъ', 'Abl' => 'л&#1123;'),
   'female' => 'fixed'
 ),
 array(
   'patterns' => array('*й', 'Берего(й)', 'Водосто(й)', 'Корро(й)',
'Коро(й)', 'Геро(й)', 'Стро(й)', 'Алло(й)', 'Градобо(й)'),
   'male' => array('Gen' => 'я', 'Dat' => 'ю', 'Acc' => 'я', 'Ins' =>
'емъ', 'Abl' => '&#1123;'),
   'female' => 'fixed'
 ),
 array(
   'patterns' => array('Бо(й)', 'Во(й)', 'Го(й)', 'До(й)', 'Ко(й)',
'Ло(й)', 'Но(й)', 'Ро(й)', 'Со(й)', 'То(й)', 'Ко(й)', 'Ло(й)',
'Уо(й)', 'Фо(й)', 'Хо(й)', 'Цо(й)', 'Чо(й)', 'Шо(й)'),
   'male' => array('Gen' => 'я', 'Dat' => 'ю', 'Acc' => 'я', 'Ins' => 'емъ', 'Abl' => '&#1123;'),
   'female' => 'fixed'
 ),
//
  array(
    'patterns' => array('*ой', '*ый'),
    'male' => array('Gen' => 'ого', 'Dat' => 'ому', 'Acc' => 'ого', 'Ins' => 'ымъ', 'Abl' => 'омъ'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*цый'),
    'male' => array('Gen' => 'цего', 'Dat' => 'цему', 'Acc' => 'цего', 'Ins' => 'цымъ', 'Abl' => 'цемъ'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*г(ой)', '*к(ой)', '*х(ой)', '*ж(ой)', '*ш(ой)', '*щ(ой)', '*ч(ой)'),
    'male' => array('Gen' => 'ого', 'Dat' => 'ому', 'Acc' => 'ого', 'Ins' => 'имъ', 'Abl' => 'омъ'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*iй'),
    'male' => array('Gen' => 'iя', 'Dat' => 'iю', 'Acc' => 'iя', 'Ins' => 'iемъ', 'Abl' => 'iи'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*г(iй)', '*к(iй)', '*х(iй)'),
    'male' => array('Gen' => 'ого', 'Dat' => 'ому', 'Acc' => 'ого', 'Ins' => 'имъ', 'Abl' => 'омъ'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*ч(iй)', '*ж(iй)', '*ш(iй)', '*щ(iй)', '*н(iй)'),
    'male' => array('Gen' => 'его', 'Dat' => 'ему', 'Acc' => 'его', 'Ins' => 'имъ', 'Abl' => 'емъ'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('Арсенi(й)', 'Гуржi(й)', 'Трухнi(й)', 'Мохi(й)', 'Топчi(й)', 'Багрi(й)', 'Тульчi(й)', 'Саланжi(й)'),
    'male' => array('Gen' => 'я', 'Dat' => 'ю', 'Acc' => 'я', 'Ins' => 'емъ', 'Abl' => 'и'),
    'female' => 'fixed'
  ),
  array(
    'patterns' => array('*ей'),
    'male' => array('Gen' => 'ея', 'Dat' => 'ею', 'Acc' => 'ея', 'Ins' => 'еемъ', 'Abl' => 'е&#1123;'),
    'female' => 'fixed'
  ),
//
  array(
   'patterns' => array('Солов(ей)', 'Вороб(ей)'),
   'male' => array('Gen' => 'ья', 'Dat' => 'ью', 'Acc' => 'ья', 'Ins' => 'ьемъ', 'Abl' => 'ь&#1123;'),
   'female' => 'fixed'
 ),
 array(
   'patterns' => array('Кротъ', 'Сукъ', 'Гнепъ', 'Кремсъ', 'Чанъ', 'Фа', 'Кустъ'),
   'male' => 'fixed',
   'female' => 'fixed',
 ),
 array(
   'patterns' => array('*ихъ', '*ыхъ'),
   'male' => 'fixed',
   'female' => 'fixed',
 ),
 array(
   'patterns' => array('*окъ', '*екъ', '*ч(ёкъ)'),
   'male' => array('Gen' => 'ка', 'Dat' => 'ку', 'Acc' => 'ка', 'Ins' => 'комъ', 'Abl' => 'к&#1123;'),
   'female' => 'fixed',
 ),
 array(
   'patterns' => array('Войтише(къ)', 'Цве(къ)', 'Дуде(къ)',
'Буче(къ)', 'Ляше(къ)', 'Ше(къ)', 'Клаче(къ)', 'Баче(къ)',
     'Корчмаре(къ)', 'Дяче(къ)', 'Цумбе(къ)', 'Собе-Пане(къ)',
'Штре(къ)', 'Сире(къ)', 'Псуе(къ)', 'Матие(къ)', 'Пане(къ)',
     'Поле(къ)', 'Саде(къ)', 'Стро(къ)', 'Ско(къ)', 'Смо(къ)',
'Воло(къ)', 'Набо(къ)', 'Бро(къ)', 'Шейнро(къ)', 'Скаче(къ)',
'Гае(къ)'),
   'male' => array('Gen' => 'ка', 'Dat' => 'ку', 'Acc' => 'ка', 'Ins'
=> 'комъ', 'Abl' => 'к&#1123;'),
   'female' => 'fixed',
 ),
 array(
   'patterns' => array('*ёкъ'),
   'male' => array('Gen' => 'ька', 'Dat' => 'ьку', 'Acc' => 'ька',
'Ins' => 'ькомъ', 'Abl' => 'ьк&#1123;'),
   'female' => 'fixed',
 ),
 array(
   'patterns' => array('*ецъ'),
   'male' => array('Gen' => 'ца', 'Dat' => 'цу', 'Acc' => 'ца', 'Ins'
=> 'цемъ', 'Abl' => 'ц&#1123;'),
   'female' => 'fixed',
 ),
 array(
   'patterns' => array('Жуклин(ецъ)', 'Гапан(ецъ)', 'Кремен(ецъ)',
'Ворон(ецъ)',
     'Рухов(ецъ)', 'От(ецъ)', 'Чигрин(ецъ)', 'Тестел(ецъ)',
'Короб(ецъ)', 'Лубен(ецъ)', 'Крав(ецъ)', 'Шв(ецъ)', 'Жн(ецъ)',
     'Бо(ецъ)', 'Титов(ецъ)', 'Скреб(ецъ)', 'Канив(ецъ)',
'Митьков(ецъ)', 'Зимов(ецъ)', 'Мул(ецъ)', 'Дон(ецъ)', 'Сидор(ецъ)',
'Туров(ецъ)',
     'Ливин(ецъ)', 'Чуднов(ецъ)', 'Мыслив(ецъ)', 'Бел(ецъ)',
'Малахов(ецъ)', 'Козуб(ецъ)', 'Казан(ецъ)', 'Якуб(ецъ)', 'Козин(ецъ)',
'Москал(ецъ)',
     'Шабан(ецъ)', 'Корни(ецъ)', 'Степан(ецъ)', 'Брагин(ецъ)',
'Левин(ецъ)', 'Руб(ецъ)', 'Кацав(ецъ)', 'Остап(ецъ)'),
   'male' => array('Gen' => 'ца', 'Dat' => 'цу', 'Acc' => 'ца', 'Ins'
=> 'цомъ', 'Abl' => 'ц&#1123;'),
   'female' => 'fixed',
 ),
 array(
   'patterns' => array('Е(лецъ)'),
   'male' => array('Gen' => 'льца', 'Dat' => 'льцу', 'Acc' => 'льца',
'Ins' => 'льцомъ', 'Abl' => 'льц&#1123;'),
   'female' => 'fixed',
 ),
 array(
   'patterns' => array('*лецъ'),
   'male' => array('Gen' => 'льца', 'Dat' => 'льцу', 'Acc' => 'льца',
'Ins' => 'льцемъ', 'Abl' => 'льц&#1123;'),
   'female' => 'fixed',
 ),
 array(
   'patterns' => array('*а(ецъ)', '*е(ецъ)', '*и(ецъ)', '*о(ецъ)', '*у(ецъ)', '*ю(ецъ)', '*я(ецъ)', '*ы(ецъ)'),
   'male' => array('Gen' => 'йца', 'Dat' => 'йцу', 'Acc' => 'йца', 'Ins' => 'йцемъ', 'Abl' => 'йце'),
   'female' => 'fixed',
 ),
 array(
   'patterns' => array('Шв(ецъ)', 'Жн(ецъ)', 'За(ецъ)', 'Б(ецъ)',
'Бразн(ецъ)', 'Гр(ецъ)', 'Ойн(ецъ)', 'Хейф(ецъ)'),
   'male' => array('Gen' => 'еца', 'Dat' => 'ецу', 'Acc' => 'еца',
'Ins' => 'ецомъ', 'Abl' => 'ец&#1123;'),
   'female' => 'fixed',
 ),
 array(
   'patterns' => array('Кеб(ецъ)', 'Ге(ецъ)'),
   'male' => array('Gen' => 'еца', 'Dat' => 'ецу', 'Acc' => 'еца', 'Ins' => 'ецемъ', 'Abl' => 'ец&#1123;'), // Ins не такой, как в предыдущем паттерне
   'female' => 'fixed',
 ),
 array(
   'patterns' => array('Вет(еръ)'),
   'male' => array('Gen' => 'ра', 'Dat' => 'ру', 'Acc' => 'ра', 'Ins' => 'ромъ', 'Abl' => 'р&#1123;'),
   'female' => 'fixed',
 ),
 array(
   'patterns' => array('*ёлъ'),
   'male' => array('Gen' => 'ла', 'Dat' => 'лу', 'Acc' => 'ла', 'Ins' => 'ломъ', 'Abl' => 'л&#1123;'),
   'female' => 'fixed',
 ),
 array(
   'patterns' => array('Каптёлъ'),
   'male' => 'fixed',
   'female' => 'fixed',
 ),

 array(
   'patterns' => array('*', '*(ъ)'),
   'male' => array('Gen' => 'а', 'Dat' => 'у', 'Acc' => 'а', 'Ins' =>
'омъ', 'Abl' => '&#1123;'),
   'female' => 'fixed',
 ),

 array(
   'patterns' => array('Варкентин(ъ)', 'Эллин(ъ)'),
   'male' => array('Gen' => 'а', 'Dat' => 'у', 'Acc' => 'а', 'Ins' => 'омъ', 'Abl' => '&#1123;'),
   'female' => 'fixed',
 ),

 array(
   'patterns' => array('*ев(ъ)', '*ов(ъ)', '*ёв(ъ)', '*ин(ъ)',
'*ын(ъ)', 'Ковалышен(ъ)'), // *ув?
   'male' => array('Gen' => 'а', 'Dat' => 'у', 'Acc' => 'а', 'Ins' => 'ымъ', 'Abl' => '&#1123;'),
   'female' => 'fixed',
 ),

 array(
   'patterns' => array('*ч(ъ)', '*ш(ъ)', '*ц(ъ)', '*щ(ъ)', '*ж(ъ)'),
   'male' => array('Gen' => 'а', 'Dat' => 'у', 'Acc' => 'а', 'Ins' => 'емъ', 'Abl' => '&#1123;'),
   'female' => 'fixed',
 ),

 array(
   'patterns' => array('Сыч(ъ)', 'Карандаш(ъ)', 'Чиж(ъ)', 'Кулиш(ъ)', 'Пыж(ъ)', 'Барабаш(ъ)', 'Пархач(ъ)', 'Деркач(ъ)', 'Грин(ъ)',
'Шин(ъ)'),
   'male' => array('Gen' => 'а', 'Dat' => 'у', 'Acc' => 'а', 'Ins' => 'омъ', 'Abl' => '&#1123;'),
   'female' => 'fixed',
 ),

 array(
   'patterns' => array("Тер-", "Нор-", "Сулима-", "Бей-", "Джан-", "Гаген-"),
   'male' => 'fixed',
   'female' => 'fixed',
 ),

); 
break;

case 21: // czech

$res['flexible_symbols'] = "QWERTYUIOPASDFGHJKLZXCVBNMqwertyuiopasdfghjklzxcvbnm";

$res['names'] = array(
  array(
    'patterns' => array('*a'),
    'male' => array('Gen' => 'y', 'Dat' => 'ovi', 'Acc' => 'u', 'Ins' => 'ou', 'Abl' => 'ovi'),
    'female' => array('Gen' => 'y', 'Dat' => '&#283;', 'Acc' => 'u', 'Ins' => 'ou', 'Abl' => '&#283;'),
  ),
  array(
    'patterns' => array('*'),
    'male' => array('Gen' => 'a', 'Dat' => 'ovi', 'Acc' => 'a', 'Ins' => 'em', 'Abl' => 'ovi'),
    'female' => 'fixed',
  ),
);


$res['surnames'] = $res['names'];

break;

case 91: // ossetian

$res['flexible_symbols'] = "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЬЭЮЯабвгдеёжзийклмнопрстуфхцчшщьэюя;";  

// &#230; = ae , &#198; = AE
$res['names'] = array(
  array( // у после гласной - полугласная (англ. w)
    'patterns' => array('*ау()', '*еу()', '*иу()', '*ыу()', '*оу()', '*уу()', '*юу()', '*яу()', '*ёу()', '*&#230;у()'),
    'male' => array('Gen' => 'ы', 'Dat' => '&#230;н', 'Dir' => 'м&#230;', 'Ine' => 'ы', 'Abl' => '&#230;й', 'Ade' => 'ыл', 'Equ' => 'ау', 'Com' => 'им&#230;'),
    'female' => array('Gen' => 'ы', 'Dat' => '&#230;н', 'Dir' => 'м&#230;', 'Ine' => 'ы', 'Abl' => '&#230;й', 'Ade' => 'ыл', 'Equ' => 'ау', 'Com' => 'им&#230;'),
  ),
  array(
    'patterns' => array('*а()', '*е()', '*и()', '*ы()', '*о()', '*у()', '*ю()', '*я()', '*ё()', '*&#230;()'),
    'male' => array('Gen' => 'йы', 'Dat' => 'й&#230;н', 'Dir' => 'м&#230;', 'Ine' => 'йы', 'Abl' => 'й&#230;', 'Ade' => 'йыл', 'Equ' => 'йау', 'Com' => 'им&#230;'),
    'female' => array('Gen' => 'йы', 'Dat' => 'й&#230;н', 'Dir' => 'м&#230;', 'Ine' => 'йы', 'Abl' => 'й&#230;', 'Ade' => 'йыл', 'Equ' => 'йау', 'Com' => 'им&#230;'),
  ),
  array(
    'patterns' => array('*'),
    'male' => array('Gen' => 'ы', 'Dat' => '&#230;н', 'Dir' => 'м&#230;', 'Ine' => 'ы', 'Abl' => '&#230;й', 'Ade' => 'ыл', 'Equ' => 'ау', 'Com' => 'им&#230;'),
    'female' => array('Gen' => 'ы', 'Dat' => '&#230;н', 'Dir' => 'м&#230;', 'Ine' => 'ы', 'Abl' => '&#230;й', 'Ade' => 'ыл', 'Equ' => 'ау', 'Com' => 'им&#230;'),
  ),
);

$res['surnames'] = array(
  array(
    'patterns' => array('*'),
    'male' => 'fixed',
    'female' => 'fixed',
  ),
);


break;

}

      $flex_config = $res;
    }
    $flex_langs[$lang_id] = $flex_config;
  }
  $flex_config = &$flex_langs[$lang_id];

  if (!$flex_config['flexible_symbols'] || empty($flex_config[$type])) return $name;

  //prepare
  if (!isset($flex_config[$type.'_sorted'])) {
    $rules = array();
    $tmp_keys = array();
    foreach ($flex_config[$type] as $i => $rule) {
      $max_len = 0;
      foreach ($rule['patterns'] as $pattern) {
        $have_asterix = strpos($pattern, '*');
        $have_hyphen = strpos($pattern, '-');
        $pattern = str_replace(array('*', '-'), array('', ''), $pattern);
        $fixed_len = strpos($pattern, '(');
        $pattern = str_replace(array('(', ')'), array('', ''), $pattern); // get clear pattern (without braces)
        $pat_len = strlen($pattern);
        if ($have_asterix !== false && $pattern == '') { // default pattern
          $last_pattern_letter = '*';
          $key = '*';
        } else if ($have_asterix === false) { // exact match pattern
          $last_pattern_letter = '*';
          $key = $pattern;
        } else {
          $last_pattern_letter = substr($pattern, -1); // organize rules by last letter
          $key = $pat_len * 100000 + $tmp_keys[$pat_len];
        }
        $rules[$last_pattern_letter][$key] = array($pat_len, $fixed_len, $pattern, $rule['male'], $rule['female'], $have_hyphen);
        $tmp_keys[$pat_len]++;
      }
    }
    foreach ($rules as $letter => $letterRules) {
      krsort($rules[$letter]); // sort rules by pattern length (long pattern goes first)
    }
    $flex_config[$type.'_sorted'] = true;
    $flex_config[$type] = $rules;
    $flex_langs[$lang_id] = $flex_config;
  } else {
    $rules = $flex_config[$type];
  }

  $default_rule = $rules['*']['*'];

  $names = explode('-', $name);
  $name_count = count($names);
  foreach ($names as $i => $name) {

    if ($rules['*'][$name] && ($rules['*'][$name][5] === false || $i != $name_count - 1)) { //exact match
      $name_len = strlen($name);
      $rule = $rules['*'][$name];
      if($rule[3 + $sex] === 'fixed' || !isset($rule[3 + $sex][$case])) continue;
      $end = $rule[3 + $sex][$case];
      $name = substr($name, 0, ($rule[1]) ? ($name_len - ($rule[0] - $rule[1])) : ($name_len - $rule[0])).$end;
      $names[$i] = $name;
      continue;
    }

    $last_letter = substr($name, -1);
    if (strpos($flex_config['flexible_symbols'], $last_letter) === false) continue;

    $letter_rules = $rules[$last_letter]; // choose patterns group by last letter
    $name_len = strlen($name);
    $last_len = 0;
    $last_sub = '';
    $pattern_found = false;
    if ($letter_rules) {
      foreach ($letter_rules as $rule) {
        if ($name_len < $rule[0]) continue;
        if ($last_len != $rule[0]) {$last_len = $rule[0]; $last_sub = substr($name, - $last_len);}
        if ($last_sub == $rule[2]) { //pattern found
          $pattern_found = true;
          if($rule[3 + $sex] === 'fixed' || !isset($rule[3 + $sex][$case])) break;
          $end = $rule[3 + $sex][$case];
          $name = substr($name, 0, ($rule[1]) ? ($name_len - ($rule[0] - $rule[1])) : ($name_len - $rule[0])).$end;
          break;
        }
      }
    }

    if (!$pattern_found) { //default pattern
      $rule = $default_rule;
      if ($rule[3 + $sex] === 'fixed') {
       $names[$i] = $name;
       continue;
      }
      $end = $rule[3 + $sex][$case];
      $name = ($rule[1]) ? substr($name, 0, ($name_len - ($rule[0] - $rule[1]))).$end : $name.$end;
    }

    $names[$i] = $name;
  }
  return implode('-', $names);
}

#endif

$langs = array (0, 1, 2, 8, 11, 19, 21, 51, 52, 91, 100, 777, 888, 999);

for ($j = 0; $j < 1000; $j++)
foreach ($langs as $i) {
  var_dump($i.vk_flex("", 'Dat', 0, 'names', $i));
  var_dump($i.vk_flex("John", 'Dat', 0, 'names', $i));
  var_dump($i.vk_flex("asdasd-", 'Dat', 0, 'names', $i));
  var_dump($i.vk_flex("Алексей", 'Dat', 0, 'names', $i));
}
