# Last changed Time-stamp: <2005-03-09 18:10:38 xtof>
# $Id: 02__Integrate.t,v 1.1 2005/05/30 19:49:13 raimc Exp $

use lib "$ENV{HOME}/Perl/L";

use Test::More tests => 1;
use Data::Dumper;
use LibODES;

my $time = 1e3;
my $printstep = 1e2;
my $model = '../../../Test/MAPK.xml';

LibODES::initializeOptions();
#LibODES::Options_PrintModel_set(1);
my $data = LibODES::Model_odeSolver(LibODES::parseModel($model),
                                    $time, $printstep);
my $refdata = join '', <DATA>;

#LibODES::CvodeData::filename::set($data, 't/02.out');
#LibODES::printConcentrationTimeCourse($data);

ok($data, $refdata);


__DATA__
#t MKKK MKKK_P MKK MKK_P MKK_PP MAPK MAPK_P MAPK_PP 
##CONCENTRATIONS
0 90 10 280 10 10 280 10 10 
10 80.718 19.282 279.662 11.6565 8.68109 279.88 11.1054 9.01507 
20 71.2642 28.7358 277.411 13.8792 8.71021 280.012 11.8575 8.13013 
30 61.6227 38.3773 273.234 16.6896 10.0766 280.05 12.4867 7.4635 
40 51.974 48.026 267.121 20.0773 12.8015 279.667 13.1953 7.13742 
50 42.6595 57.3405 259.105 23.9659 16.9288 278.551 14.1698 7.27936 
60 34.1268 65.8732 249.3 28.2199 22.4801 276.391 15.5743 8.03503 
70 26.824 73.176 237.921 32.6736 29.4052 272.885 17.532 9.58258 
80 21.0614 78.9386 225.277 37.1633 37.5598 267.755 20.1063 12.1388 
90 16.9094 83.0906 211.724 41.5522 46.7239 260.755 23.293 15.9518 
100 14.2049 85.7951 197.611 45.7411 56.6475 251.69 27.0276 21.2823 
110 12.6573 87.3427 183.237 49.6676 67.0955 240.417 31.2035 28.3799 
120 11.968 88.032 168.831 53.2969 77.8717 226.844 35.6935 37.4625 
130 11.8958 88.1042 154.567 56.6109 88.8223 210.931 40.3654 48.7039 
140 12.2677 87.7323 140.571 59.5997 99.8292 192.682 45.0883 62.2299 
150 12.9654 87.0346 126.944 62.2558 110.801 172.152 49.7289 78.1193 
160 13.9086 86.0914 113.765 64.5708 121.664 149.453 54.1386 96.408 
170 15.0412 84.9588 101.107 66.5326 132.36 124.781 58.1269 117.092 
180 16.3233 83.6767 89.0358 68.1242 142.84 98.4659 61.4077 140.126 
190 17.7251 82.2749 77.6162 69.3226 153.061 71.1078 63.4765 165.416 
200 19.2241 80.7759 66.9154 70.0978 162.987 43.9396 63.2817 192.779 
210 20.8021 79.1979 57.0041 70.4122 172.584 19.9002 58.2764 221.823 
220 22.4438 77.5562 47.957 70.222 181.821 5.25343 43.315 251.432 
230 24.1335 75.8665 39.8504 69.4794 190.67 1.45795 20.4964 278.046 
240 25.85 74.15 32.756 68.1397 199.104 0.609171 5.43806 293.953 
250 27.5681 72.4319 26.7278 66.1727 207.1 0.221537 1.9475 297.831 
260 29.275 70.725 21.7863 63.5801 214.634 0.138369 1.52071 298.341 
270 30.9687 69.0313 17.9015 60.4146 221.684 0.1216 1.43194 298.446 
280 32.6494 67.3506 14.9825 56.7904 228.227 0.11353 1.38165 298.505 
290 34.3176 65.6824 12.8843 52.8745 234.241 0.107289 1.3406 298.552 
300 35.9736 64.0264 11.4337 48.8584 239.708 0.102085 1.3055 298.592 
310 37.6176 62.3824 10.4606 44.9243 244.615 0.0977221 1.27548 298.627 
320 39.2497 60.7503 9.82182 41.2201 248.958 0.094079 1.24999 298.656 
330 40.8701 59.1299 9.40916 37.8496 252.741 0.0910567 1.22853 298.68 
340 42.4788 57.5212 9.14793 34.8739 255.978 0.0885723 1.21067 298.701 
350 44.0759 55.9241 8.99017 32.3189 258.691 0.0865547 1.19603 298.717 
360 45.6611 54.3389 8.90739 30.1833 260.909 0.0849423 1.18424 298.731 
370 47.2346 52.7654 8.88425 28.4472 262.669 0.0836814 1.17497 298.741 
380 48.796 51.204 8.91379 27.0793 264.007 0.0827254 1.16793 298.749 
390 50.3453 49.6547 8.99405 26.0424 264.964 0.0820342 1.16284 298.755 
400 51.8823 48.1177 9.12577 25.2979 265.576 0.0815737 1.15947 298.759 
410 53.4065 46.5935 9.31096 24.809 265.88 0.0813154 1.15762 298.761 
420 54.9179 45.0821 9.55216 24.5423 265.906 0.0812363 1.15712 298.762 
430 56.4159 43.5841 9.85204 24.4686 265.679 0.081318 1.15784 298.761 
440 57.9003 42.0997 10.2134 24.5628 265.224 0.0815462 1.15967 298.759 
450 59.3705 40.6295 10.639 24.8044 264.557 0.0819105 1.16252 298.756 
460 60.8262 39.1738 11.1321 25.1758 263.692 0.0824039 1.16634 298.751 
470 62.2667 37.7333 11.696 25.663 262.641 0.083022 1.1711 298.746 
480 63.6914 36.3086 12.3343 26.2539 261.412 0.083763 1.17676 298.739 
490 65.0998 34.9002 13.0515 26.9385 260.01 0.0846272 1.18333 298.732 
500 66.4912 33.5088 13.852 27.7083 258.44 0.0856169 1.1908 298.724 
510 67.8647 32.1353 14.7411 28.5555 256.703 0.0867363 1.19921 298.714 
520 69.2197 30.7803 15.7242 29.4735 254.802 0.0879912 1.20857 298.703 
530 70.5552 29.4448 16.8075 30.4558 252.737 0.0893889 1.21892 298.692 
540 71.8702 28.1298 17.997 31.4964 250.507 0.0909384 1.23032 298.679 
550 73.1638 26.8362 19.2994 32.5892 248.111 0.0926505 1.24281 298.665 
560 74.4348 25.5652 20.7212 33.7285 245.55 0.0945373 1.25646 298.649 
570 75.6822 24.3178 22.2692 34.9083 242.822 0.096613 1.27134 298.632 
580 76.9046 23.0954 23.9499 36.1228 239.927 0.0988933 1.28754 298.614 
590 78.1008 21.8992 25.7695 37.366 236.864 0.101396 1.30513 298.593 
600 79.2694 20.7306 27.7341 38.632 233.634 0.104142 1.32421 298.572 
610 80.4089 19.5911 29.849 39.9151 230.236 0.107154 1.34491 298.548 
620 81.5178 18.4822 32.119 41.2094 226.672 0.110457 1.36732 298.522 
630 82.5945 17.4055 34.548 42.5094 222.943 0.11408 1.39158 298.494 
640 83.6374 16.3626 37.1393 43.8097 219.051 0.118055 1.41783 298.464 
650 84.6448 15.3552 39.8951 45.1053 215 0.122418 1.44623 298.431 
660 85.6151 14.3849 42.8164 46.3915 210.792 0.127212 1.47695 298.396 
670 86.5465 13.4535 45.9035 47.6638 206.433 0.132483 1.51018 298.357 
680 87.4376 12.5624 49.1553 48.9181 201.927 0.138283 1.54612 298.316 
690 88.2867 11.7133 52.5699 50.151 197.279 0.144673 1.58501 298.27 
700 89.0926 10.9074 56.1441 51.359 192.497 0.151721 1.6271 298.221 
710 89.8539 10.1461 59.8739 52.5393 187.587 0.159506 1.67268 298.168 
720 90.5697 9.43033 63.7542 53.6893 182.557 0.168119 1.72207 298.11 
730 91.2393 8.76074 67.7791 54.8069 177.414 0.177662 1.77562 298.047 
740 91.8623 8.13774 71.942 55.89 172.168 0.188256 1.83374 297.978 
750 92.4387 7.56133 76.2358 56.9371 166.827 0.200039 1.89687 297.903 
760 92.9689 7.03114 80.6526 57.9466 161.401 0.213173 1.96554 297.821 
770 93.4536 6.54635 85.1843 58.9172 155.898 0.227847 2.04032 297.732 
780 93.8942 6.10577 89.8227 59.8478 150.33 0.244282 2.12188 297.634 
790 94.2922 5.70777 94.5594 60.737 144.704 0.262741 2.21099 297.526 
800 94.6496 5.35041 99.386 61.5838 139.03 0.283531 2.30852 297.408 
810 94.9686 5.03143 104.294 62.3869 133.319 0.307021 2.4155 297.277 
820 95.2517 4.74834 109.277 63.1451 127.578 0.333652 2.53311 297.133 
830 95.5015 4.49848 114.325 63.8568 121.818 0.363952 2.66272 296.973 
840 95.7209 4.27912 119.433 64.5202 116.047 0.398562 2.80594 296.795 
850 95.9125 4.08751 124.593 65.1334 110.274 0.438255 2.96467 296.597 
860 96.0791 3.92093 129.799 65.6941 104.507 0.483981 3.14114 296.375 
870 96.2232 3.77679 135.045 66.1993 98.7556 0.536898 3.33796 296.125 
880 96.3474 3.65261 140.326 66.646 93.0277 0.598438 3.55824 295.843 
890 96.4539 3.54612 145.638 67.0301 87.3322 0.670373 3.80564 295.524 
900 96.5448 3.45522 150.975 67.3471 81.6783 0.754906 4.08452 295.161 
910 96.622 3.37801 156.333 67.5917 76.0757 0.854787 4.39998 294.745 
920 96.6872 3.3128 161.708 67.7575 70.5346 0.973465 4.75803 294.269 
930 96.7419 3.25811 167.097 67.8369 65.0663 1.11527 5.16571 293.719 
940 96.7873 3.21266 172.496 67.8211 59.6833 1.28564 5.6311 293.083 
950 96.8247 3.17533 177.901 67.6995 54.3994 1.49144 6.1634 292.345 
960 96.8548 3.1452 183.31 67.4597 49.2307 1.74126 6.77281 291.486 
970 96.8785 3.12151 188.718 67.0869 44.1955 2.04585 7.47023 290.484 
980 96.8963 3.10365 194.121 66.5637 39.3148 2.41849 8.2667 289.315 
990 96.9088 3.09116 199.517 65.8697 34.6132 2.87545 9.17253 287.952 
1000 96.9163 3.08369 204.9 64.981 30.1189 3.43619 10.196 286.368 
##CONCENTRATIONS
#t MKKK MKKK_P MKK MKK_P MKK_PP MAPK MAPK_P MAPK_PP 



