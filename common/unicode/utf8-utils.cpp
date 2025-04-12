// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/unicode/utf8-utils.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

void string_to_utf8 (const char *s, int *v) {
  int *tv = v;
#define CHECK(x) if (!(x)) {v = tv; break;}

  int a, b, c, d;

  while (*s) {
    a = (unsigned char)*s++;
    if ((a & 0x80) == 0) {
      *v++ = a;
    } else if ((a & 0x40) == 0) {
      CHECK(0);
    } else if ((a & 0x20) == 0) {
      b = (unsigned char)*s++;
      CHECK((b & 0xc0) == 0x80);
      CHECK((a & 0x1e) > 0);
      *v++ = ((a & 0x1f) << 6) | (b & 0x3f);
    } else if ((a & 0x10) == 0) {
      b = (unsigned char)*s++;
      CHECK((b & 0xc0) == 0x80);
      c = (unsigned char)*s++;
      CHECK((c & 0xc0) == 0x80);
      CHECK(((a & 0x0f) | (b & 0x20)) > 0);
      *v++ = ((a & 0x0f) << 12) | ((b & 0x3f) << 6) | (c & 0x3f);
    } else if ((a & 0x08) == 0) {
      b = (unsigned char)*s++;
      CHECK((b & 0xc0) == 0x80);
      c = (unsigned char)*s++;
      CHECK((c & 0xc0) == 0x80);
      d = (unsigned char)*s++;
      CHECK((d & 0xc0) == 0x80);
      CHECK(((a & 0x07) | (b & 0x30)) > 0);
      *v++ = ((a & 0x07) << 18) | ((b & 0x3f) << 12) | ((c & 0x3f) << 6) | (d & 0x3f);
    } else {
      CHECK(0);
    }
  }
  *v = 0;
#undef CHECK
}

void string_to_utf8_len (const char *s, int s_len, int *v) {
  int *tv = v;
  const char *s_end = s + s_len;
#define CHECK(x) if (!(x)) {v = tv; break;}

  int a, b, c, d;

  while (s < s_end) {
    a = (unsigned char)*s++;
    if ((a & 0x80) == 0) {
      *v++ = a;
    } else if ((a & 0x40) == 0) {
      CHECK(0);
    } else if ((a & 0x20) == 0) {
      CHECK(s < s_end);
      b = (unsigned char)*s++;
      CHECK((b & 0xc0) == 0x80);
      CHECK((a & 0x1e) > 0);
      *v++ = ((a & 0x1f) << 6) | (b & 0x3f);
    } else if ((a & 0x10) == 0) {
      CHECK(s < s_end);
      b = (unsigned char)*s++;
      CHECK((b & 0xc0) == 0x80);
      CHECK(s < s_end);
      c = (unsigned char)*s++;
      CHECK((c & 0xc0) == 0x80);
      CHECK(((a & 0x0f) | (b & 0x20)) > 0);
      *v++ = ((a & 0x0f) << 12) | ((b & 0x3f) << 6) | (c & 0x3f);
    } else if ((a & 0x08) == 0) {
      CHECK(s < s_end);
      b = (unsigned char)*s++;
      CHECK((b & 0xc0) == 0x80);
      CHECK(s < s_end);
      c = (unsigned char)*s++;
      CHECK((c & 0xc0) == 0x80);
      CHECK(s < s_end);
      d = (unsigned char)*s++;
      CHECK((d & 0xc0) == 0x80);
      CHECK(((a & 0x07) | (b & 0x30)) > 0);
      *v++ = ((a & 0x07) << 18) | ((b & 0x3f) << 12) | ((c & 0x3f) << 6) | (d & 0x3f);
    } else {
      CHECK(0);
    }
  }
  *v = 0;
#undef CHECK
}

void html_string_to_utf8 (const char *s, int *v) {
  string_to_utf8 (s, v);

  int i, j;
  for (i = j = 0; v[i]; i++) {
    if (v[i] == '&') {
      if (v[i + 1] == 'a' && v[i + 2] == 'm' && v[i + 3] == 'p' && v[i + 4] == ';') {
        i += 4;
        v[j++] = '&';
      } else if (v[i + 1] == 'l' && v[i + 2] == 't' && v[i + 3] == ';') {
        i += 3;
        v[j++] = '<';
      } else if (v[i + 1] == 'g' && v[i + 2] == 't' && v[i + 3] == ';') {
        i += 3;
        v[j++] = '>';
      } else if (v[i + 1] == 'q' && v[i + 2] == 'u' && v[i + 3] == 'o' && v[i + 4] == 't' && v[i + 5] == ';') {
        i += 5;
        v[j++] = '"';
      } else if (v[i + 1] == '#') {
        int r = 0, ti = i;
        if (v[i + 2] == 'x') {
          for (i += 3; ('0' <= v[i] && v[i] <= '9') || ((unsigned int)((v[i] | 0x20) - 'a') < 6); i++) {
            if ('0' <= v[i] && v[i] <= '9') {
              r = r * 16 + v[i] - '0';
            } else {
              r = r * 16 + (v[i] | 0x20) - 'a' + 10;
            }
          }
        } else {
          for (i += 2; '0' <= v[i] && v[i] <= '9'; i++) {
            r = r * 10 + v[i] - '0';
          }
        }

        if (v[i] != ';') {
          i = ti;
          v[j++] = '&';
        } else {
          if ((unsigned int)r <= 0x10ffff) {
            v[j++] = r;
          }
        }
      } else {
        v[j++] = '&';
      }
    } else {
      v[j++] = v[i];
    }
  }
  v[j++] = 0;
}

int put_string_utf8 (const int *v, char *s) {
  const char *s_begin = s;
  while (*v) {
    s += put_char_utf8 ((unsigned int)(*v), s);
    v++;
  }
  *s = 0;
  return s - s_begin;
}


int simplify_character (int c) {
  switch (c) {
    case 193:
    case 258:
    case 7854:
    case 7862:
    case 7856:
    case 7858:
    case 7860:
    case 461:
    case 194:
    case 7844:
    case 7852:
    case 7846:
    case 7848:
    case 7850:
    case 196:
    case 7840:
    case 192:
    case 7842:
    case 256:
    case 260:
    case 197:
    case 506:
    case 195:
    case 198:
    case 508:
      return 65;
    case 7684:
    case 385:
    case 666:
    case 606:
      return 66;
    case 262:
    case 268:
    case 199:
    case 264:
    case 266:
    case 390:
    case 663:
      return 67;
    case 270:
    case 7698:
    case 7692:
    case 394:
    case 7694:
    case 498:
    case 453:
    case 272:
    case 208:
    case 497:
    case 452:
      return 68;
    case 201:
    case 276:
    case 282:
    case 202:
    case 7870:
    case 7878:
    case 7872:
    case 7874:
    case 7876:
    case 203:
    case 278:
    case 7864:
    case 200:
    case 7866:
    case 274:
    case 280:
    case 7868:
    case 400:
    case 399:
      return 69;
    case 401:
      return 70;
    case 500:
    case 286:
    case 486:
    case 290:
    case 284:
    case 288:
    case 7712:
    case 667:
      return 71;
    case 7722:
    case 292:
    case 7716:
    case 294:
      return 72;
    case 205:
    case 300:
    case 463:
    case 206:
    case 207:
    case 304:
    case 7882:
    case 204:
    case 7880:
    case 298:
    case 302:
    case 296:
    case 306:
      return 73;
    case 308:
      return 74;
    case 310:
    case 7730:
    case 408:
    case 7732:
      return 75;
    case 313:
    case 573:
    case 317:
    case 315:
    case 7740:
    case 7734:
    case 7736:
    case 7738:
    case 319:
    case 456:
    case 321:
    case 455:
      return 76;
    case 7742:
    case 7744:
    case 7746:
      return 77;
    case 323:
    case 327:
    case 325:
    case 7754:
    case 7748:
    case 7750:
    case 504:
    case 413:
    case 7752:
    case 459:
    case 209:
    case 458:
      return 78;
    case 211:
    case 334:
    case 465:
    case 212:
    case 7888:
    case 7896:
    case 7890:
    case 7892:
    case 7894:
    case 214:
    case 7884:
    case 336:
    case 210:
    case 7886:
    case 416:
    case 7898:
    case 7906:
    case 7900:
    case 7902:
    case 7904:
    case 332:
    case 415:
    case 490:
    case 216:
    case 510:
    case 213:
    case 338:
    case 630:
      return 79;
    case 222:
      return 80;
      return 81;
    case 340:
    case 344:
    case 342:
    case 7768:
    case 7770:
    case 7772:
    case 7774:
    case 641:
      return 82;
    case 346:
    case 352:
    case 350:
    case 348:
    case 536:
    case 7776:
    case 7778:
    case 7838:
      return 83;
    case 356:
    case 354:
    case 7792:
    case 538:
    case 7788:
    case 7790:
    case 358:
      return 84;
    case 218:
    case 364:
    case 467:
    case 219:
    case 220:
    case 471:
    case 473:
    case 475:
    case 469:
    case 7908:
    case 368:
    case 217:
    case 7910:
    case 431:
    case 7912:
    case 7920:
    case 7914:
    case 7916:
    case 7918:
    case 362:
    case 370:
    case 366:
    case 360:
      return 85;
      return 86;
    case 7810:
    case 372:
    case 7812:
    case 7808:
    case 684:
      return 87;
      return 88;
    case 221:
    case 374:
    case 376:
    case 7822:
    case 7924:
    case 7922:
    case 435:
    case 7926:
    case 562:
    case 7928:
      return 89;
    case 377:
    case 381:
    case 379:
    case 7826:
    case 7828:
    case 437:
      return 90;
    case 225:
    case 259:
    case 7855:
    case 7863:
    case 7857:
    case 7859:
    case 7861:
    case 462:
    case 226:
    case 7845:
    case 7853:
    case 7847:
    case 7849:
    case 7851:
    case 228:
    case 7841:
    case 224:
    case 7843:
    case 257:
    case 261:
    case 229:
    case 507:
    case 227:
    case 230:
    case 509:
    case 593:
    case 592:
    case 594:
      return 97;
    case 7685:
    case 595:
    case 223:
      return 98;
    case 263:
    case 269:
    case 231:
    case 265:
    case 597:
    case 267:
      return 99;
    case 271:
    case 7699:
    case 7693:
    case 599:
    case 7695:
    case 273:
    case 598:
    case 676:
    case 499:
    case 675:
    case 677:
    case 454:
    case 240:
      return 100;
    case 233:
    case 277:
    case 283:
    case 234:
    case 7871:
    case 7879:
    case 7873:
    case 7875:
    case 7877:
    case 235:
    case 279:
    case 7865:
    case 232:
    case 7867:
    case 275:
    case 281:
    case 7869:
    case 658:
    case 495:
    case 659:
    case 600:
    case 604:
    case 605:
    case 601:
    case 602:
      return 101;
    case 402:
    case 383:
    case 681:
    case 64257:
    case 64258:
    case 643:
    case 646:
    case 645:
    case 644:
      return 102;
    case 501:
    case 287:
    case 487:
    case 291:
    case 285:
    case 289:
    case 608:
    case 7713:
    case 609:
    case 611:
      return 103;
    case 7723:
    case 293:
    case 7717:
    case 614:
    case 7830:
    case 295:
    case 615:
    case 613:
    case 686:
    case 687:
      return 104;
    case 237:
    case 301:
    case 464:
    case 238:
    case 239:
    case 7883:
    case 236:
    case 7881:
    case 299:
    case 303:
    case 616:
    case 297:
    case 617:
    case 305:
    case 307:
      return 105;
    case 496:
    case 309:
    case 669:
    case 567:
    case 607:
      return 106;
    case 311:
    case 7731:
    case 409:
    case 7733:
    case 312:
    case 670:
      return 107;
    case 314:
    case 410:
    case 620:
    case 318:
    case 316:
    case 7741:
    case 7735:
    case 7737:
    case 7739:
    case 320:
    case 619:
    case 621:
    case 322:
    case 411:
    case 622:
    case 457:
    case 682:
    case 683:
      return 108;
    case 7743:
    case 7745:
    case 7747:
    case 625:
    case 623:
    case 624:
      return 109;
    case 329:
    case 324:
    case 328:
    case 326:
    case 7755:
    case 7749:
    case 7751:
    case 505:
    case 626:
    case 7753:
    case 627:
    case 241:
    case 460:
    case 331:
    case 330:
      return 110;
    case 243:
    case 335:
    case 466:
    case 244:
    case 7889:
    case 7897:
    case 7891:
    case 7893:
    case 7895:
    case 246:
    case 7885:
    case 337:
    case 242:
    case 7887:
    case 417:
    case 7899:
    case 7907:
    case 7901:
    case 7903:
    case 7905:
    case 333:
    case 491:
    case 248:
    case 511:
    case 245:
    case 603:
    case 596:
    case 629:
    case 664:
    case 339:
      return 111;
    case 632:
    case 254:
      return 112;
    case 672:
      return 113;
    case 341:
    case 345:
    case 343:
    case 7769:
    case 7771:
    case 7773:
    case 638:
    case 7775:
    case 636:
    case 637:
    case 639:
    case 633:
    case 635:
    case 634:
      return 114;
    case 347:
    case 353:
    case 351:
    case 349:
    case 537:
    case 7777:
    case 7779:
    case 642:
      return 115;
    case 357:
    case 355:
    case 7793:
    case 539:
    case 7831:
    case 7789:
    case 7791:
    case 648:
    case 359:
    case 680:
    case 679:
    case 678:
    case 647:
      return 116;
    case 649:
    case 250:
    case 365:
    case 468:
    case 251:
    case 252:
    case 472:
    case 474:
    case 476:
    case 470:
    case 7909:
    case 369:
    case 249:
    case 7911:
    case 432:
    case 7913:
    case 7921:
    case 7915:
    case 7917:
    case 7919:
    case 363:
    case 371:
    case 367:
    case 361:
    case 650:
      return 117;
    case 651:
    case 652:
      return 118;
    case 7811:
    case 373:
    case 7813:
    case 7809:
    case 653:
      return 119;
      return 120;
    case 253:
    case 375:
    case 255:
    case 7823:
    case 7925:
    case 7923:
    case 436:
    case 7927:
    case 563:
    case 7929:
    case 654:
      return 121;
    case 378:
    case 382:
    case 657:
    case 380:
    case 7827:
    case 7829:
    case 656:
    case 438:
      return 122;

    case 9398 ... 9423:
      return c - 9398 + 'a';
    case 9424 ... 9449:
      return c - 9424 + 'a';
    case 9372 ... 9397:
      return c - 9372 + 'a';
    case 65313 ... 65338:
      return c - 65313 + 'a';
    case 65345 ... 65370:
      return c - 65345 + 'a';
    case 120250 ... 120275:
      return c - 120250 + 'a';
    case 9312 ... 9320:
      return c - 9312 + '1';
    case 9332 ... 9340:
      return c - 9332 + '1';
    case 9352 ... 9360:
      return c - 9352 + '1';
    case 65296 ... 65305:
      return c - 65296 + '0';

    case 170:      return 'a';
    case 178:      return '2';
    case 179:      return '3';
    case 185:      return '1';
    case 186:      return 'o';
    case 688:      return 'h';
    case 690:      return 'j';
    case 691:      return 'r';
    case 695:      return 'w';
    case 696:      return 'y';
    case 737:      return 'l';
    case 738:      return 's';
    case 739:      return 'x';
    case 7468:      return 'a';
    case 7470:      return 'b';
    case 7472:      return 'd';
    case 7473:      return 'e';
    case 7475:      return 'g';
    case 7476:      return 'h';
    case 7477:      return 'i';
    case 7478:      return 'j';
    case 7479:      return 'k';
    case 7480:      return 'l';
    case 7481:      return 'm';
    case 7482:      return 'n';
    case 7484:      return 'o';
    case 7486:      return 'p';
    case 7487:      return 'r';
    case 7488:      return 't';
    case 7489:      return 'u';
    case 7490:      return 'w';
    case 7491:      return 'a';
    case 7495:      return 'b';
    case 7496:      return 'd';
    case 7497:      return 'e';
    case 7501:      return 'g';
    case 7503:      return 'k';
    case 7504:      return 'm';
    case 7506:      return 'o';
    case 7510:      return 'p';
    case 7511:      return 't';
    case 7512:      return 'u';
    case 7515:      return 'v';
    case 7522:      return 'i';
    case 7523:      return 'r';
    case 7524:      return 'u';
    case 7525:      return 'v';
    case 8304:      return '0';
    case 8305:      return 'i';
    case 8308:      return '4';
    case 8309:      return '5';
    case 8310:      return '6';
    case 8311:      return '7';
    case 8312:      return '8';
    case 8313:      return '9';
    case 8319:      return 'n';
    case 8320:      return '0';
    case 8321:      return '1';
    case 8322:      return '2';
    case 8323:      return '3';
    case 8324:      return '4';
    case 8325:      return '5';
    case 8326:      return '6';
    case 8327:      return '7';
    case 8328:      return '8';
    case 8329:      return '9';
    case 8336:      return 'a';
    case 8337:      return 'e';
    case 8338:      return 'o';
    case 8339:      return 'x';
    case 8450:      return 'c';
    case 8458:      return 'g';
    case 8459:      return 'h';
    case 8460:      return 'h';
    case 8461:      return 'h';
    case 8462:      return 'h';
    case 8464:      return 'i';
    case 8465:      return 'i';
    case 8466:      return 'l';
    case 8467:      return 'l';
    case 8469:      return 'n';
    case 8473:      return 'p';
    case 8474:      return 'q';
    case 8475:      return 'r';
    case 8476:      return 'r';
    case 8477:      return 'r';
    case 8484:      return 'z';
    case 8488:      return 'z';
    case 8490:      return 'k';
    case 8492:      return 'b';
    case 8493:      return 'c';
    case 8495:      return 'e';
    case 8496:      return 'e';
    case 8497:      return 'f';
    case 8499:      return 'm';
    case 8500:      return 'o';
    case 8505:      return 'i';
    case 8517:      return 'd';
    case 8518:      return 'd';
    case 8519:      return 'e';
    case 8520:      return 'i';
    case 8521:      return 'j';
    case 8544:      return 'i';
    case 8548:      return 'v';
    case 8553:      return 'x';
    case 8556:      return 'l';
    case 8557:      return 'c';
    case 8558:      return 'd';
    case 8559:      return 'm';
    case 8560:      return 'i';
    case 8564:      return 'v';
    case 8569:      return 'x';
    case 8572:      return 'l';
    case 8573:      return 'c';
    case 8574:      return 'd';
    case 8575:      return 'm';
    case 9450:      return '0';

    default:
      return c;
  }
}

const int _s_1__[] = {97, 0};
const int _v_1__[] = {1072, 0};
const int _s_2__[] = {98, 0};
const int _v_2__[] = {1073, 0};
const int _s_3__[] = {99, 0};
const int _v_3__[] = {1082, 0};
const int _s_4__[] = {99, 104, 0};
const int _v_4__[] = {1095, 0};
const int _s_5__[] = {100, 0};
const int _v_5__[] = {1076, 0};
const int _s_6__[] = {101, 0};
const int _v_6__[] = {1077, 0};
const int _s_7__[] = {101, 105, 0};
const int _v_7__[] = {1077, 1081, 0};
const int _s_8__[] = {101, 121, 0};
const int _v_8__[] = {1077, 1081, 0};
const int _s_9__[] = {102, 0};
const int _v_9__[] = {1092, 0};
const int _s_10__[] = {103, 0};
const int _v_10__[] = {1075, 0};
const int _s_11__[] = {104, 0};
const int _v_11__[] = {1093, 0};
const int _s_12__[] = {105, 0};
const int _v_12__[] = {1080, 0};
const int _s_13__[] = {105, 97, 0};
const int _v_13__[] = {1080, 1103, 0};
const int _s_14__[] = {105, 121, 0};
const int _v_14__[] = {1080, 1081, 0};
const int _s_15__[] = {106, 0};
const int _v_15__[] = {1081, 0};
const int _s_16__[] = {106, 111, 0};
const int _v_16__[] = {1077, 0};
const int _s_17__[] = {106, 117, 0};
const int _v_17__[] = {1102, 0};
const int _s_18__[] = {106, 97, 0};
const int _v_18__[] = {1103, 0};
const int _s_19__[] = {107, 0};
const int _v_19__[] = {1082, 0};
const int _s_20__[] = {107, 104, 0};
const int _v_20__[] = {1093, 0};
const int _s_21__[] = {108, 0};
const int _v_21__[] = {1083, 0};
const int _s_22__[] = {109, 0};
const int _v_22__[] = {1084, 0};
const int _s_23__[] = {110, 0};
const int _v_23__[] = {1085, 0};
const int _s_24__[] = {111, 0};
const int _v_24__[] = {1086, 0};
const int _s_25__[] = {112, 0};
const int _v_25__[] = {1087, 0};
const int _s_26__[] = {113, 0};
const int _v_26__[] = {1082, 0};
const int _s_27__[] = {114, 0};
const int _v_27__[] = {1088, 0};
const int _s_28__[] = {115, 0};
const int _v_28__[] = {1089, 0};
const int _s_29__[] = {115, 104, 0};
const int _v_29__[] = {1096, 0};
const int _s_30__[] = {115, 104, 99, 104, 0};
const int _v_30__[] = {1097, 0};
const int _s_31__[] = {115, 99, 104, 0};
const int _v_31__[] = {1097, 0};
const int _s_32__[] = {116, 0};
const int _v_32__[] = {1090, 0};
const int _s_33__[] = {116, 115, 0};
const int _v_33__[] = {1094, 0};
const int _s_34__[] = {117, 0};
const int _v_34__[] = {1091, 0};
const int _s_35__[] = {118, 0};
const int _v_35__[] = {1074, 0};
const int _s_36__[] = {119, 0};
const int _v_36__[] = {1074, 0};
const int _s_37__[] = {120, 0};
const int _v_37__[] = {1082, 1089, 0};
const int _s_38__[] = {121, 0};
const int _v_38__[] = {1080, 0};
const int _s_39__[] = {121, 111, 0};
const int _v_39__[] = {1077, 0};
const int _s_40__[] = {121, 117, 0};
const int _v_40__[] = {1102, 0};
const int _s_41__[] = {121, 97, 0};
const int _v_41__[] = {1103, 0};
const int _s_42__[] = {122, 0};
const int _v_42__[] = {1079, 0};
const int _s_43__[] = {122, 104, 0};
const int _v_43__[] = {1078, 0};
const int _s_44__[] = {1072, 0};
const int _v_44__[] = {97, 0};
const int _s_45__[] = {1073, 0};
const int _v_45__[] = {98, 0};
const int _s_46__[] = {1074, 0};
const int _v_46__[] = {118, 0};
const int _s_47__[] = {1075, 0};
const int _v_47__[] = {103, 0};
const int _s_48__[] = {1076, 0};
const int _v_48__[] = {100, 0};
const int _s_49__[] = {1077, 0};
const int _v_49__[] = {101, 0};
const int _s_50__[] = {1105, 0};
const int _v_50__[] = {101, 0};
const int _s_51__[] = {1078, 0};
const int _v_51__[] = {122, 104, 0};
const int _s_52__[] = {1079, 0};
const int _v_52__[] = {122, 0};
const int _s_53__[] = {1080, 0};
const int _v_53__[] = {105, 0};
const int _s_54__[] = {1080, 1081, 0};
const int _v_54__[] = {121, 0};
const int _s_55__[] = {1080, 1103, 0};
const int _v_55__[] = {105, 97, 0};
const int _s_56__[] = {1081, 0};
const int _v_56__[] = {121, 0};
const int _s_57__[] = {1082, 0};
const int _v_57__[] = {107, 0};
const int _s_58__[] = {1082, 1089, 0};
const int _v_58__[] = {120, 0};
const int _s_59__[] = {1083, 0};
const int _v_59__[] = {108, 0};
const int _s_60__[] = {1084, 0};
const int _v_60__[] = {109, 0};
const int _s_61__[] = {1085, 0};
const int _v_61__[] = {110, 0};
const int _s_62__[] = {1086, 0};
const int _v_62__[] = {111, 0};
const int _s_63__[] = {1087, 0};
const int _v_63__[] = {112, 0};
const int _s_64__[] = {1088, 0};
const int _v_64__[] = {114, 0};
const int _s_65__[] = {1089, 0};
const int _v_65__[] = {115, 0};
const int _s_66__[] = {1090, 0};
const int _v_66__[] = {116, 0};
const int _s_67__[] = {1091, 0};
const int _v_67__[] = {117, 0};
const int _s_68__[] = {1092, 0};
const int _v_68__[] = {102, 0};
const int _s_69__[] = {1093, 0};
const int _v_69__[] = {107, 104, 0};
const int _s_70__[] = {1094, 0};
const int _v_70__[] = {116, 115, 0};
const int _s_71__[] = {1095, 0};
const int _v_71__[] = {99, 104, 0};
const int _s_72__[] = {1096, 0};
const int _v_72__[] = {115, 104, 0};
const int _s_73__[] = {1097, 0};
const int _v_73__[] = {115, 104, 99, 104, 0};
const int _s_74__[] = {1098, 0};
const int _v_74__[] = {0};
const int _s_75__[] = {1099, 0};
const int _v_75__[] = {121, 0};
const int _s_76__[] = {1100, 0};
const int _v_76__[] = {0};
const int _s_77__[] = {1101, 0};
const int _v_77__[] = {101, 0};
const int _s_78__[] = {1102, 0};
const int _v_78__[] = {121, 117, 0};
const int _s_79__[] = {1103, 0};
const int _v_79__[] = {121, 97, 0};

int translit_string_utf8_from_en_to_ru (int *input, int *output) {

#define TEST(s, v)                                \
  k = 0;                                          \
  while (input[i + k] && input[i + k] == s[k]) {  \
    k++;                                          \
  }                                               \
  if (!s[k]) {                                    \
    match_v = v;                                  \
    match_s = s;                                  \
  }

  int i = 0, j = 0, k = 0;
  int const *match_s, *match_v;

  while (input[i]) {
    match_s = NULL;
    match_v = NULL;
    switch (input[i]) {
      case 97://a
        //a --> а
        TEST(_s_1__, _v_1__);
        break;
      case 98://b
        //b --> б
        TEST(_s_2__, _v_2__);
        break;
      case 99://c
        //c --> к
        TEST(_s_3__, _v_3__);
        //ch --> ч
        TEST(_s_4__, _v_4__);
        break;
      case 100://d
        //d --> д
        TEST(_s_5__, _v_5__);
        break;
      case 101://e
        //e --> е
        TEST(_s_6__, _v_6__);
        //ei --> ей
        TEST(_s_7__, _v_7__);
        //ey --> ей
        TEST(_s_8__, _v_8__);
        break;
      case 102://f
        //f --> ф
        TEST(_s_9__, _v_9__);
        break;
      case 103://g
        //g --> г
        TEST(_s_10__, _v_10__);
        break;
      case 104://h
        //h --> х
        TEST(_s_11__, _v_11__);
        break;
      case 105://i
        //i --> и
        TEST(_s_12__, _v_12__);
        //ia --> ия
        TEST(_s_13__, _v_13__);
        //iy --> ий
        TEST(_s_14__, _v_14__);
        break;
      case 106://j
        //j --> й
        TEST(_s_15__, _v_15__);
        //jo --> е
        TEST(_s_16__, _v_16__);
        //ju --> ю
        TEST(_s_17__, _v_17__);
        //ja --> я
        TEST(_s_18__, _v_18__);
        break;
      case 107://k
        //k --> к
        TEST(_s_19__, _v_19__);
        //kh --> х
        TEST(_s_20__, _v_20__);
        break;
      case 108://l
        //l --> л
        TEST(_s_21__, _v_21__);
        break;
      case 109://m
        //m --> м
        TEST(_s_22__, _v_22__);
        break;
      case 110://n
        //n --> н
        TEST(_s_23__, _v_23__);
        break;
      case 111://o
        //o --> о
        TEST(_s_24__, _v_24__);
        break;
      case 112://p
        //p --> п
        TEST(_s_25__, _v_25__);
        break;
      case 113://q
        //q --> к
        TEST(_s_26__, _v_26__);
        break;
      case 114://r
        //r --> р
        TEST(_s_27__, _v_27__);
        break;
      case 115://s
        //s --> с
        TEST(_s_28__, _v_28__);
        //sh --> ш
        TEST(_s_29__, _v_29__);
        //shch --> щ
        TEST(_s_30__, _v_30__);
        //sch --> щ
        TEST(_s_31__, _v_31__);
        break;
      case 116://t
        //t --> т
        TEST(_s_32__, _v_32__);
        //ts --> ц
        TEST(_s_33__, _v_33__);
        break;
      case 117://u
        //u --> у
        TEST(_s_34__, _v_34__);
        break;
      case 118://v
        //v --> в
        TEST(_s_35__, _v_35__);
        break;
      case 119://w
        //w --> в
        TEST(_s_36__, _v_36__);
        break;
      case 120://x
        //x --> кс
        TEST(_s_37__, _v_37__);
        break;
      case 121://y
        //y --> и
        TEST(_s_38__, _v_38__);
        //yo --> е
        TEST(_s_39__, _v_39__);
        //yu --> ю
        TEST(_s_40__, _v_40__);
        //ya --> я
        TEST(_s_41__, _v_41__);
        break;
      case 122://z
        //z --> з
        TEST(_s_42__, _v_42__);
        //zh --> ж
        TEST(_s_43__, _v_43__);
        break;
      default:
        break;
    }

    if (match_s != NULL) {
      k = 0;
      while (match_v[k]) {
        output[j++] = match_v[k++];
      }
      k = 0;
      while (match_s[k]) {
        i++, k++;
      }
    }
    else {
      output[j++] = input[i++];
    }
  }

  output[j] = 0;

  return j;

#undef TEST
}

int translit_string_utf8_from_ru_to_en (int *input, int *output) {

#define TEST(s, v)                                \
  k = 0;                                          \
  while (input[i + k] && input[i + k] == s[k]) {  \
    k++;                                          \
  }                                               \
  if (!s[k]) {                                    \
    match_v = v;                                  \
    match_s = s;                                  \
  }

  int i = 0, j = 0, k = 0;
  int const *match_s, *match_v;

  while (input[i]) {
    match_s = NULL;
    match_v = NULL;
    switch (input[i]) {
      case 1072://а
        //а --> a
        TEST(_s_44__, _v_44__);
        break;
      case 1073://б
        //б --> b
        TEST(_s_45__, _v_45__);
        break;
      case 1074://в
        //в --> v
        TEST(_s_46__, _v_46__);
        break;
      case 1075://г
        //г --> g
        TEST(_s_47__, _v_47__);
        break;
      case 1076://д
        //д --> d
        TEST(_s_48__, _v_48__);
        break;
      case 1077://е
        //е --> e
        TEST(_s_49__, _v_49__);
        break;
      case 1105://ё
        //ё --> e
        TEST(_s_50__, _v_50__);
        break;
      case 1078://ж
        //ж --> zh
        TEST(_s_51__, _v_51__);
        break;
      case 1079://з
        //з --> z
        TEST(_s_52__, _v_52__);
        break;
      case 1080://и
        //и --> i
        TEST(_s_53__, _v_53__);
        //ий --> y
        TEST(_s_54__, _v_54__);
        //ия --> ia
        TEST(_s_55__, _v_55__);
        break;
      case 1081://й
        //й --> y
        TEST(_s_56__, _v_56__);
        break;
      case 1082://к
        //к --> k
        TEST(_s_57__, _v_57__);
        //кс --> x
        TEST(_s_58__, _v_58__);
        break;
      case 1083://л
        //л --> l
        TEST(_s_59__, _v_59__);
        break;
      case 1084://м
        //м --> m
        TEST(_s_60__, _v_60__);
        break;
      case 1085://н
        //н --> n
        TEST(_s_61__, _v_61__);
        break;
      case 1086://о
        //о --> o
        TEST(_s_62__, _v_62__);
        break;
      case 1087://п
        //п --> p
        TEST(_s_63__, _v_63__);
        break;
      case 1088://р
        //р --> r
        TEST(_s_64__, _v_64__);
        break;
      case 1089://с
        //с --> s
        TEST(_s_65__, _v_65__);
        break;
      case 1090://т
        //т --> t
        TEST(_s_66__, _v_66__);
        break;
      case 1091://у
        //у --> u
        TEST(_s_67__, _v_67__);
        break;
      case 1092://ф
        //ф --> f
        TEST(_s_68__, _v_68__);
        break;
      case 1093://х
        //х --> kh
        TEST(_s_69__, _v_69__);
        break;
      case 1094://ц
        //ц --> ts
        TEST(_s_70__, _v_70__);
        break;
      case 1095://ч
        //ч --> ch
        TEST(_s_71__, _v_71__);
        break;
      case 1096://ш
        //ш --> sh
        TEST(_s_72__, _v_72__);
        break;
      case 1097://щ
        //щ --> shch
        TEST(_s_73__, _v_73__);
        break;
      case 1098://ъ
        //ъ -->
        TEST(_s_74__, _v_74__);
        break;
      case 1099://ы
        //ы --> y
        TEST(_s_75__, _v_75__);
        break;
      case 1100://ь
        //ь -->
        TEST(_s_76__, _v_76__);
        break;
      case 1101://э
        //э --> e
        TEST(_s_77__, _v_77__);
        break;
      case 1102://ю
        //ю --> yu
        TEST(_s_78__, _v_78__);
        break;
      case 1103://я
        //я --> ya
        TEST(_s_79__, _v_79__);
        break;
    }

    if (match_s != NULL) {
      k = 0;
      while (match_v[k]) {
        output[j++] = match_v[k++];
      }
      k = 0;
      while (match_s[k]) {
        i++, k++;
      }
    }
    else {
      output[j++] = input[i++];
    }
  }

  output[j] = 0;

  return j;

#undef TEST
}

int convert_language (int x) {
  switch (x) {
    case 113://q->й
      return 1081;
    case 119://w->ц
      return 1094;
    case 101://e->у
      return 1091;
    case 114://r->к
      return 1082;
    case 116://t->е
      return 1077;
    case 121://y->н
      return 1085;
    case 117://u->г
      return 1075;
    case 105://i->ш
      return 1096;
    case 111://o->щ
      return 1097;
    case 112://p->з
      return 1079;
    case 91://[->х
      return 1093;
    case 93://]->ъ
      return 1098;
    case 97://a->ф
      return 1092;
    case 115://s->ы
      return 1099;
    case 100://d->в
      return 1074;
    case 102://f->а
      return 1072;
    case 103://g->п
      return 1087;
    case 104://h->р
      return 1088;
    case 106://j->о
      return 1086;
    case 107://k->л
      return 1083;
    case 108://l->д
      return 1076;
    case 59://;->ж
      return 1078;
    case 39://'->э
      return 1101;
    case 122://z->я
      return 1103;
    case 120://x->ч
      return 1095;
    case 99://c->с
      return 1089;
    case 118://v->м
      return 1084;
    case 98://b->и
      return 1080;
    case 110://n->т
      return 1090;
    case 109://m->ь
      return 1100;
    case 44://,->б
      return 1073;
    case 46://.->ю
      return 1102;
    case 96://`->е
      return 1077;
    case 81://Q->Й
      return 1049;
    case 87://W->Ц
      return 1062;
    case 69://E->У
      return 1059;
    case 82://R->К
      return 1050;
    case 84://T->Е
      return 1045;
    case 89://Y->Н
      return 1053;
    case 85://U->Г
      return 1043;
    case 73://I->Ш
      return 1064;
    case 79://O->Щ
      return 1065;
    case 80://P->З
      return 1047;
    case 123://{->Х
      return 1061;
    case 125://}->Ъ
      return 1066;
    case 65://A->Ф
      return 1060;
    case 83://S->Ы
      return 1067;
    case 68://D->В
      return 1042;
    case 70://F->А
      return 1040;
    case 71://G->П
      return 1055;
    case 72://H->Р
      return 1056;
    case 74://J->О
      return 1054;
    case 75://K->Л
      return 1051;
    case 76://L->Д
      return 1044;
    case 58://:->Ж
      return 1046;
    case 34://"->Э
      return 1069;
    case 90://Z->Я
      return 1071;
    case 88://X->Ч
      return 1063;
    case 67://C->С
      return 1057;
    case 86://V->М
      return 1052;
    case 66://B->И
      return 1048;
    case 78://N->Т
      return 1058;
    case 77://M->Ь
      return 1068;
    case 60://<->Б
      return 1041;
    case 62://>->Ю
      return 1070;
    case 126://~->Е
      return 1045;
    case 1081://й->q
      return 113;
    case 1094://ц->w
      return 119;
    case 1091://у->e
      return 101;
    case 1082://к->r
      return 114;
    case 1077://е->t
      return 116;
    case 1085://н->y
      return 121;
    case 1075://г->u
      return 117;
    case 1096://ш->i
      return 105;
    case 1097://щ->o
      return 111;
    case 1079://з->p
      return 112;
    case 1093://х->[
      return 91;
    case 1098://ъ->]
      return 93;
    case 1092://ф->a
      return 97;
    case 1099://ы->s
      return 115;
    case 1074://в->d
      return 100;
    case 1072://а->f
      return 102;
    case 1087://п->g
      return 103;
    case 1088://р->h
      return 104;
    case 1086://о->j
      return 106;
    case 1083://л->k
      return 107;
    case 1076://д->l
      return 108;
    case 1078://ж->;
      return 59;
    case 1101://э->'
      return 39;
    case 1103://я->z
      return 122;
    case 1095://ч->x
      return 120;
    case 1089://с->c
      return 99;
    case 1084://м->v
      return 118;
    case 1080://и->b
      return 98;
    case 1090://т->n
      return 110;
    case 1100://ь->m
      return 109;
    case 1073://б->,
      return 44;
    case 1102://ю->.
      return 46;
    case 1105://ё->`
      return 96;
    case 1049://Й->Q
      return 81;
    case 1062://Ц->W
      return 87;
    case 1059://У->E
      return 69;
    case 1050://К->R
      return 82;
    case 1045://Е->T
      return 84;
    case 1053://Н->Y
      return 89;
    case 1043://Г->U
      return 85;
    case 1064://Ш->I
      return 73;
    case 1065://Щ->O
      return 79;
    case 1047://З->P
      return 80;
    case 1061://Х->{
      return 123;
    case 1066://Ъ->}
      return 125;
    case 1060://Ф->A
      return 65;
    case 1067://Ы->S
      return 83;
    case 1042://В->D
      return 68;
    case 1040://А->F
      return 70;
    case 1055://П->G
      return 71;
    case 1056://Р->H
      return 72;
    case 1054://О->J
      return 74;
    case 1051://Л->K
      return 75;
    case 1044://Д->L
      return 76;
    case 1046://Ж->:
      return 58;
    case 1069://Э->"
      return 34;
    case 1071://Я->Z
      return 90;
    case 1063://Ч->X
      return 88;
    case 1057://С->C
      return 67;
    case 1052://М->V
      return 86;
    case 1048://И->B
      return 66;
    case 1058://Т->N
      return 78;
    case 1068://Ь->M
      return 77;
    case 1041://Б-><
      return 60;
    case 1070://Ю->>
      return 62;
    case 1025://Ё->~
      return 126;
    case 48://0->0
      return 48;
    case 49://1->1
      return 49;
    case 50://2->2
      return 50;
    case 51://3->3
      return 51;
    case 52://4->4
      return 52;
    case 53://5->5
      return 53;
    case 54://6->6
      return 54;
    case 55://7->7
      return 55;
    case 56://8->8
      return 56;
    case 57://9->9
      return 57;
    default:
      if (x >= 128) {
        return x;
      }
      return 32;
  }
}

int has_bad_symbols (int *v_s) {
  while (v_s && *v_s) {
    if ((*v_s >= 123 && *v_s <= 125) || (*v_s >= 91 && *v_s <= 93) || (*v_s >= 58 && *v_s <= 64) || (*v_s >= 34 && *v_s <= 47 && *v_s != 43 /*+*/)) {
      return 1;
    }
    v_s++;
  }
  return 0;
}
