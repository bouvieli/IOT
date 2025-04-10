                                    Bare - metal programming

Dans ce document, on peut retrouver toutes mes connaissances autour du bare-metal programming :
Les différentes mémoires et le fonctionnement de la partie Hardware.

- Comment fonctionnent QEMU et les différents fichiers sources ?
QEMU est un émulateur qui simule la machine (CPU, mémoires et périphériques) et peut exécuter du code binaire pour une architecture cible.
Quand je lance ma machine simulée cette dernière ne contient rien. Elle ne sait ni quoi exécuter ni où. C'est pourquoi quand on lance QEMU il faut lui associer un fichier .elf contenant les informations sur l'organisation mémoire, le point d'entrée et le code machine.
Contenu et travail du .elf : En lisant le Makefile, je vois que make run va nous permettre de lancer QEMU avec le fichier kernel.elf. Ce fichier contient tous les exécutables (exception.o, main.o, startup.o, uart.o) qui ont été regroupés grâce au linker kernel.ld qui permet de définir l'organisation mémoire. Ce dernier définit les sections de stockage du code exécutable (.text), des variables initialisées (.data), des variables non initialisées (.bss) et de la pile (stack). Il permet à QEMU de savoir à quelle adresse dans la RAM il va devoir exécuter le code de startup.o et exception.o et le reste des exécutables.

- Mais si la machine ne connaît rien, où est chargé et exécuté ce .elf ?
Lorsque l'on lance QEMU avec notre fichier kernel.elf ce dernier est chargé et exécuté à l'adresse définie par le linker pour nous 0x0. Parmi le contenu de kernel.elf c'est le contenu de startup.s qui est exécuté en premier.

- Qu'est-ce que startup.s ?
startup initialise notre système pour qu'il puisse exécuter notre noyau (main.c).
Au début de ce dernier, on retrouve la définition des constantes qui définissent les modes possibles pour le processeur (utilisateur, système, interruption) et de désactiver les interruptions. La constante qui correspond au mode actuel et celles qui correspondent aux autorisations d'interruptions sont stockées dans le registre CPSR. Registre qui permet de connaître l'état du processeur.
On configure alors le processeur en mode système avec les interruptions désactivées. Avant de charger l'adresse du début de la pile dans le registre sp et d'initialiser les variables non initialisées à 0. Puis de sauter à l'adresse de la fonction _start qui se trouve dans notre main.c.

- Que fait _start ?
Il commence par vérifier que la pile n'est pas remplie. Puis initialise les UARTs avant de faire une boucle qui attend de recevoir quelque chose dans le port uart et de le renvoyer.
Initialiser les UARTs revient à enregistrer leurs numéros et adresses de départ dans un tableau.
Ensuite on appelle uart_enable du port UART0 qui nous permettra de gérer les interruptions mais qui pour l'instant ne fait rien. Enfin on rentre dans une boucle qui fait appel à uart_receive et uart_send sur UART0.

uart_receive regarde si le 4e bit du registre de flag est à 1 en appliquant un "et" bit à bit entre ce dernier et un masque où seul le quatrième bit est à 1. Tant que le résultat de l'opération est 1, c'est-à-dire tant que le 4e bit du registre de flag est à 1, alors le FIFO de réception est vide et on attend. Lorsque le FIFO de réception n'est plus vide on charge le caractère présent à l'adresse donnée en argument depuis l'adresse du FIFO.

uart_send regarde si le 5e bit du registre de flag est à 1 ; si oui cela signifie que le registre FIFO est plein. On attend tant que c'est le cas et écrit un caractère donné en argument à l'adresse du FIFO.


- Pour pouvoir accéder au port uart et au registre fifo et flags il faut donc connaître leur adresse et les configurer dans uart-mmio.h
J'ai pu trouver ces adresses dans la documentation. On a donc :
#define UART0_BASE_ADDRESS ((void*)0x101F1000)
#define UART1_BASE_ADDRESS ((void*)0x101F2000)
#define UART2_BASE_ADDRESS ((void*)0x101F3000)

// adresse du registre fifo
#define UART_DR 0x00

// adresse du registre de flag
#define UART_FR 0x18

Chaque UART a une adresse de départ et à partir de celle-ci, l'adresse des registres flag et fifo représente un décalage par rapport à cette adresse. Le registre flag de l'UART0 se trouve donc à l'adresse 0x101F1018.

- Où est-ce qu'on écrit réellement ? Dans quelle mémoire ? Quelles sont les adresses où j'écris ?
Pour comprendre cela je me suis penché sur le tableau de mappage des adresses que l'on retrouve slide 22 du cours (Lecture1.pdf)

Chaque système a une procédure de mappage des adresses afin d'attribuer différentes plages d'adresses à différentes fonctionnalités de ce dernier. Selon la plage d'adresses utilisée on accède à différentes mémoires physiques. Ainsi, les registres de UART sont stockés dans une mémoire réservée aux périphériques, sur ces derniers. Elle est mappée et accessible grâce aux adresses ci-dessus.


- Quelles sont les différentes mémoires ? Quels sont leurs rôles et qui y accède ?

Un SoC est une puce qui contient tout ce qu’un ordinateur aurait besoin pour fonctionner : CPU, mémoire, périphériques, bus de communication et FPGA.
La mémoire du SoC est divisée en deux : la RAM et la ROM.
La RAM correspond à une mémoire vive et volatile. C'est-à-dire que cette dernière est vidée lorsque l'on arrête le système. Dans la RAM on retrouve l'OCM qui est interne au processeur et sert à stocker des données critiques et d’amorçage comme le firmware ou le bootloader au démarrage. Elle est très rapide et ne contient que quelques Ko-Mo. On retrouve aussi parmi la RAM, la DDR, qui est externe et connectée via bus mémoire. Elle permet de stocker le code et les applications en cours d'exécution et les données volumineuses. Cette dernière est plus lente que l'OCM mais plus grande (Go).

Ensuite, on retrouve la mémoire ROM ou flash qui est non volatile. Ici, c'est une mémoire Quad-SPI qui est utilisée. Il existe d'autres mémoires ROM comme la mémoire NAND qui est moins chère mais moins rapide. Ces deux mémoires flash sont plus lentes que les mémoires RAM mais ont de plus grandes capacités. Elles permettent de stocker l'OS, le firmware et les fichiers utilisateurs. Ces dernières sont connectées au serveur grâce aux plages d'adresses SMC qui permettent de connecter les mémoires externes au serveur.

Dans le système de mappage des adresses on retrouve aussi des plages d'adresses pour la partie PL qui correspond à la partie FPGA d'un SoC. La FPGA est une puce qui permet de reprogrammer du matériel. On retrouve aussi la plage IOP qui correspond aux adresses des périphériques (comme UART), la plage des registres SLCR qui permettent de configurer les composants du SoC. Par exemple de modifier la fréquence du CPU, des bus, d'activer ou de désactiver des composants. Et la plage d'adresse des registres PS qui contrôlent et configurent le processeur, les périphériques et la mémoire. On y retrouve les CR pour configurer le processeur, IR pour gérer les interruptions, les SR qui connaissent l'état du processeur et le MMR qui gère les interactions entre l'espace d'adressage du processeur et l'espace physique de la mémoire.

- Qu'est-ce qui se passe lorsque j'allume mon ordinateur ? Qu'est-ce qui se lance ? Qui est responsable de quelle partie logiciel, matériel ?
Quand on lance une machine, le CPU commence par charger le firmware dans l'OCM et l'exécuter. Ce dernier sert à initialiser le matériel en configurant la mémoire, activant les périphériques puis il charge le bootloader toujours dans l'OCM qui lui est responsable de charger le système d'exploitation ou l'application principale et de la démarrer en donnant le contrôle à celui-ci via un saut à l'adresse où il commence son exécution. Le bootloader peut être contenu dans le firmware. Pour nous tout cela est dans kernel.elf qui est lancé lorsque l'on lance notre machine émulée avec QEMU. Tout est directement chargé en mémoire grâce au kernel.ld. Il n'y a donc pas de chargement en mémoire effectué. La mémoire est configurée par les fichiers .s et le saut est effectué dans startup.s avec :
ldr r3,=_start
blx r3


- Comment fonctionnent les interruptions ? Pourquoi les utiliser ?

Utiliser les interruptions permet que plutôt que le CPU soit en attente de recevoir des données et qu'il tourne en permanence, il s'active pour gérer l'envoi des données quand il reçoit une interruption.
Quand le CPU reçoit une exception, il connaît l'adresse à laquelle il doit aller en fonction de l'exception reçue. Si c'est une exception d'interruption IRQ, il exécute l'instruction à l'adresse 0x18. Le fichier exception.s détermine les différentes instructions exécutées en fonction de l'exception. On voit que l'instruction à l'adresse 0x18 est :
ldr pc, irq_handler_addr
Il va donc sauter à l'adresse irq_handler_addr qui correspond à l'adresse de la fonction _isr_handler. Et va donc exécuter cette fonction pour gérer l'interruption. Pour exécuter l'interruption et revenir au même état qu’avant sa détection, il faut donc le sauvegarder.
Pour exécuter l'exception, le CPU doit désactiver ses interruptions pour ne pas en avoir de nouvelles pendant qu'il traite l'interruption en cours.
Le processeur sauvegarde automatiquement l'adresse du PC dans le link register et le CPSR dans SPSR_irq et passe en mode interruption. Mais l'instruction ayant commencé, l'adresse du PC est l'adresse de l'instruction suivante. Il faut donc enlever 4 octets à LR avant d'enregistrer l'état des registres, d'appeler la fonction isr() qui traite l'interruption.

Avant d'avoir une interruption qui arrive au CPU, elle doit être générée par le port UART et transmise vers le VIC puis vers le CPU.



- Comment l'envoi des données peut générer une interruption UART ?

En UART, il y a deux sortes d'interruptions : RX qui concerne l'interruption à la réception (en fonction de l'état de la FIFO de réception, s'active lorsque suffisamment de données sont reçues) et TX sur la transmission (en fonction de l'état de la FIFO de transmission, s'active quand suffisamment d'espace est libéré dans la FIFO de transmission).
Pour générer et gérer une interruption UART, il existe différents registres :
    UARTIMSC qui permet d'activer et de désactiver des interruptions
    UARTRIS qui permet de savoir si une interruption a eu lieu ou non et est en attente
    UARTMIS pour savoir lesquelles sont réellement actives, c'est-à-dire en attente et activées

- Comment celles-ci sont envoyées au VIC ?

Le VIC vient vérifier le statut de ses lignes d'interruptions ; si un des bits est à 1, cela signifie qu'une interruption a eu lieu. Si cette interruption est désactivée, on l'ignore, sinon on la transmet au CPU.
Pour le VIC, chaque périphérique a un numéro d'interruption qui permet de vérifier l'état de cette dernière sur chaque registre.
Par exemple, si le 12e bit du registre VICIRQSTATUS est 1, cela signifie qu'une interruption a été transmise depuis le port UART0 et est active. Donc qu'elle doit être transmise au CPU.


Le transfert d'interruption entre l'UART et le VIC se fait via une ligne IRQ dédiée par un signal électrique. Pareil entre le VIC et le CPU. Si l'UART est configuré pour générer une interruption à la réception, alors dès qu'une donnée est reçue il génère une interruption vers le VIC. Si le VIC est configuré avec les interruptions correspondantes activées, alors il transmet automatiquement l'interruption au CPU.
Le CPU va alors effectuer la fonction de gestion de cette interruption. Nous, on veut que cette dernière permette d'afficher le caractère tapé à l'écran. La fonction callback doit donc correspondre à l'exécution de :
uart_receive(UART0, &c);
uart_send(UART0, c);

Vu que la réception crée une exception, l’écriture dans le terminal va faire sortir de la fonction _start. Le début en sera quand même exécuté pour initialiser l'UART et son relevé d'exception.


- Quand activer/desactiver les interruptions pour le cpu et le vic?

Au lancement, je commence par appeler vic_setup_irqs() qui initialise le matériel pour qu’il puisse recevoir des interruptions et initialise les handlers pour indiquer qu’aucune interruption n’est encore traitée. Je nettoie les interruptions actives et je désactive les interruptions pour le VIC.

Ensuite, j’initialise les UARTs et j’active les interruptions RX pour UART0. J’active les interruptions pour le VIC en associant au numéro d’interruption de UART0 la fonction de traitement de cette interruption, qui ici fait l’affichage. Ensuite, j’active les interruptions CPU. J’ai laissé la boucle for, mais son contenu ne sera pas exécuté car à la réception une exception sera levée.

- Problème: une seule intéruption s'execute 

Le problème venais que je m'étais le bit de VICINTCLEAR à 1 en pensant que cela permettrait de remettre le bit du VICSTATUS à 0 alors qu'il remet celui du VICINTENABLE et empêche donc les nouvelles intéruptions. Aussi je mettais le bit de UART_ICR à 1 ce qui remet bien le bit de UARTMIS à 0 mais n'est pas indispenssable car quand la fifo est vide il est remis à 0 automatiquement. 

- Reconnaissance/Traitement des caractère scpéciaux 
Comme je récupère sur 32 bits, les caractères spéciaux comme les caractères accentués sont déjà transmis. Ces derniers étant encodés sur 2 octets, si j’utilisais des uint8_t, une seule interruption ne suffirait pas à les transmettre. Pour ce qui est des flèches, elles sont encodées sur 3 octets, et avec un read sur 32 bits je récupère tout en une seule lecture.

Ces commandes sont directement interprétées par notre terminal, mais ce n’est pas le cas pour toutes, notamment les Ctrl + lettre.

Sans interprétation, si je fais une combinaison de touches comme Ctrl+C, rien ne s’affiche car je me contente de transmettre l’octet correspondant au terminal (0x03), qui n’affiche rien. J’ai donc ajouté des interprétations pour qu’en fonction de la touche sur laquelle j’appuie, j’aie quelque chose qui s’affiche. Je peux par exemple effacer les caractères précédents, sauter une ligne, effacer le contenu du terminal, afficher ou rendre invisible le curseur.

- Modification: 
Certains caractères et combinaisons de touches ont un code ASCII qui est envoyé à la machine via l'UART quand on les réalise. C'est le cas des lettres et des combinaisons telles que Ctrl + lettre. Cependant, certains codes ne sont pas directement interprétés par le terminal. Il faut donc créer une interprétation lors de la lecture par la machine avant de transmettre au terminal. Par exemple, quand je reçois 0x03, qui est le code ASCII de Ctrl + C, j'envoie au terminal \033[H\033[J afin qu'il efface tout le contenu de ce dernier.
Aussi, certaines touches sont représentées par des combinaisons sur plusieurs bits. Il faut donc lire chaque bit un à un et transmettre l’entièreté de la combinaison pour que le terminal sache ce qu’on lui demande. C’est le cas des flèches. Pour bouger le curseur vers la gauche, on reçoit \033, puis [, et enfin D, et on transmet \x1B[D.
Même si on lit des uint_32 = 4 octets, on est en réalité obligé de lire en trois fois, car avec l’UART, les bits sont transmis un à un.

Maintenant, avec l’exécution de ces interprétations, la durée de l’exécution est plus longue et aucune autre interruption ne peut être lancée, donc il y a un risque de perdre des octets. Il faut donc avoir une fonction de traitement de l’interruption plus courte en s’occupant uniquement du device qui génère l’interruption et en laissant l’exécution de l’interprétation et l’envoi vers le terminal dans la boucle principale, et donc en dehors du traitement de l’interruption.
On peut alors stocker les données envoyées par le device au moment de l’interruption et les récupérer dans la boucle principale.
Pour cela, il faut soit utiliser une liste, mais pour laquelle on ne peut pas lire et écrire en même temps, et où il faudra donc bloquer les interruptions au moment de la lecture dans la boucle principale,
soit utiliser un buffer circulaire pour lequel on peut avoir le double accès. Si le prochain endroit où écrire est égal à l’endroit où lire, alors le buffer est plein. Et si l’endroit où lire et écrire sont égaux, alors il est vide.

J’ai rencontré un problème qui faisait que j’avais > qui bouclait à l’infini.
En fait, dans mon code qui interprète les caractères tapés en ligne de commande, dans les cas pas spéciaux j’affiche le caractère, et si on dépasse la largeur autorisée dans le terminal, je vais à la ligne et j’affiche >.
Ici :
for (;;) {
    
    process_buffer();
    core_disable_irqs();
    if (ring_is_empty()) {
      //core_enable_irqs(); 
      core_halt();
    }
    
  } mes interruptions n’étaient jamais autorisées, donc start s’exécutait en boucle,
et dans process_buffer je lis la ring qui contient "", puisqu’elle n’était jamais remplie.
Puis je l’interprète en dépassant la largeur maximum et en affichant donc >.
En fait, mon core_halt ne réactivait pas les interruptions.
J’ai donc ajouté core_enable_irqs(); à l’intérieur. Maintenant, je peux envoyer et avoir les caractères qui s’affichent comme précédemment.

Maintenant, nous voulons rendre nos données accessibles à une application qui viendrait agir sur ces données.
Pour cela, au lieu de renvoyer directement les données du buffer circulaire au terminal, j’appellerai le listener de lecture de l’application une fois que des données seront disponibles, et il ne sera pas rappelé tant qu’elle n’aura pas vidé le buffer, puis que de nouvelles données seront disponibles.
Les données seront renvoyées au terminal lorsque l’application les renverra via le buffer circulaire d’écriture.
On met aussi un listener qui est appelé quand le buffer a de la place pour écrire.
Ensuite, les données sont envoyées du buffer vers le terminal grâce à une interruption tx.
Quand la FIFO tx a suffisamment de place, cela déclenche une interruption via laquelle on vient lire les données du buffer d’écriture et les écrire dans la FIFO qui va les transmettre au terminal.

À son démarrage, notre application vient initialiser l’UART afin de prévenir la machine des fonctions qu’elle devra appeler pour la prévenir qu’elle peut lire des données depuis le buffer de lecture ou écrire dans le buffer d’écriture.
Elle alloue aussi la place pour la structure qu’elle va utiliser pour traiter les données (cookie).
Ensuite, quand des données seront disponibles, la machine appellera le listener de lecture, et l’application viendra récupérer les données dans le buffer de lecture en les stockant dans sa structure avant de les traiter.

J’ai réussi à avancer le travail jusqu’à permettre à l’application de lire les données dans le buffer de lecture après que le programme principal a appelé son read listener.
Cette dernière renvoie ensuite directement les données vers l’UART sans passer par le buffer d’écriture.




Faire un Makefile: 
Un makefile est composé de commandes sous la forme : 
    cible: dependance    exemple->  mon_executable: ex.o main.o
           commandes                                 gcc -o mon-executable ex.o main.o
                                    ex.o:           ex.c
                                                    gcc -o ex.o -c ex.c


Pour améliorer le makefiles on utilise des commandes:
all: tous les executables à produire (qd on fait make c cette commande qui s'execute)
clean: pour supprimer tous les fichiers intermediaires comme les .o
mrproper : pour supprimer tout ce qui peut être régénéré afin de reconstruire entièrement le projet

Pour eviter de rmultiplier les réécriture on utilise des variables:
CC: pour le compilateur utilisé
CFLAGS: pour les options de compilation
LDFLAGS: pour les options de liens
EXEC: pour le nom des executables 
$@ Le nom de la cible
$< Le nom de la première dépendance
$^ La liste des dépendances
$? La liste des dépendances plus récentes que la cible

et des règles d'inférences pour créer des règles génériques.
%.o: %.c
    commandes 
tous les points c seront compilés avec la même règle de compilation. Même compilateur, même options de compilation et même dépendances

Enfin pour éviter de lister la liste des fichiers objects dans les dépendances des executables on peut utiliser OBJ= $(SRC:.c=.o) avec 
SRC = liste des fichiers sources du projet







Utilisation de gdb pour debugger:
- Lancer GDB avec kernel.elf : 
ouvrir deux terminaux avec dans le premier "make debug"
et dans le deuxieme "arm-none-eabi-gdb build/kernel.elf" puis "target remote localhost:1234"

- dans gdb on peut :

Positionner des break points sur des fonctions (break nom_fonction), sur une ligne dans le fichier en cours d'execution (break numero). On peut les supprimer avec delete numero_break_point et les listers avec info breakpoints. 

Controler l'execution :
c pour executer jusqu'au prochain breakpoint
n pour executer la ligne et passer à la suivante (sans entrer dans les fonctions)
s pour executer et entrer dans les fonctions appelées
finish pour finir la fonction en cours
q pour quitter

Naviguer dans le code:
list pour afficher le code autour de la ligne actuelle
list numéro_ligne pour afficher le code autour de la ligne souhaitée
list fonction pour afficher le code autour de la fonction souhaitée






  