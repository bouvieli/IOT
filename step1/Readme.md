                                    Bare - metal programming

Dans ce document, on peut retrouver toutes mes connaissances autour du bare-metal programming : 
Les différentes mémoires et le fonctionnement de la partie Hardware. 

- Comment fonctionnent QEMU et les différents fichiers sources ?

QEMU est un emulateur qui simule la machine (CPU, mémoires et périphériques) et peut executer du code binaire pour une architecture cible. 
Quand je lance ma machine simulée cette dernière ne contient rien. Elle ne sait ni quoi executer ni où. C'est pourquoi quand on lance QEMU il faut lui associer un fichier .elf contenant les informations sur l'organisation mémoire, le point d'entrée et le code machine.
Contenu et travail du .elf: En lissant le Makefile, je vois que make run va nous permettre de lancer QEMU avec le fichier kernel.elf. Ce fichier contient tous les executables (exception.o, main.o, startup.o, uart.o) qui on étaient regroupé grace au linker kernel.ld qui permet de définir l'organisation mémoire . Ce dernier définit les sections de stockage du code executable (.text), des variables initialisées (.data), des variables non initialisées (.bss) et de la pile (stack). Il permet à QEMU de savoir à quelle adresse dans la ram il va devoir executer le code de startup.o et exception.o et le reste des executables.

- Mais si la machine ne connait rien où est chargé et executé ce .elf ?
Lorsque l'on lance quemu avec notre fichier kernel.elf ce dernier est chargé et exécuté à l'adresse défini par le linker pour nous 0x0. Parmis le contenu de kernel.elf c'est le contenu de startup.s qui est exécuté en premier. 

- Qu'est ce que startup.s ?
startup initialise notre systeme pour qu'il puisse executer notre noyau (main.c). 
Au début de ce dernier, on retrouve la définition des constantes qui définisse les modes possible pour le processeur (utilisateur,system, interruption) et de désactiver les interruptions . La constante qui correspond au mode actuel et celles qui correspondent aux autorisations d'interruptions sont stockées dans le le registre CPSR. Registre qui permet de connaitre l'état du processeur.
On configure alors le processeur en mode systeme avec les interruptions désactivées. Avant de charger l'adresse du début de la pile dans le registre sp et d'initialisé les variables non initialisées à 0. Puis de sauter à l'adresse de la fonction _start qui ce trouve dans notre main.c.

- que fais _start ?
il commence par vérifier que la pile n'est pas rempli. Puis initialise les UARTs avant de faire une boucle qui attend de recevoir quelque chose dans le port uart et de le renvoyer.
Initialisé les UARTs revient à enregistrer leurs numéros et adresses de départ dans un tableau.
ensuite on appel uart_enable du port UART0 qui nous permettra des gerer les interruptions mais qui pour l'instant de fait rien. Enfin on rentre dans une boucle qui fait appel à uart_receive et uart_send sur uartO.

uart_receive regarde si le 4 eme bit du registre de flag est à 1 en appliquant un "et" bit à bit entre ce dernier et un masque où seul le quatrième bit est à un 1. Tant que le resultat de l'opération est 1. C'est à dire tant que le 4 eme bit du registre de flag est à 1 alors le fifo de reception est vide et on attend. Lorsque le fifo de reception n'est plus vide on charge le caractère présent à l'adresse donnée en argument depuis l'adresse du fifo. 

uart_send regarde si le 5 eme bit du registre de flag est à 1 si oui cela signifie que le registre fifo est pleins. On attend tant que c'est le cas et écrit un caractaire donnée en argument à l'adresse du fifo.


- Pour pouvoir accéder au port uart et au registre fifo et flags il faut donc connaitre leur adresse et les configurer dans uart-mmio.h
J'ai pu trouver ces adresses dans la documentation. On a donc :

#define UART0_BASE_ADDRESS ((void*)0x101F1000)
#define UART1_BASE_ADDRESS ((void*)0x101F2000) 
#define UART2_BASE_ADDRESS ((void*)0x101F3000) 

// adresse du registre fifo
#define UART_DR 0x00 

//adresse du registre de flag
#define UART_FR 0x18

Chaque uart à une adresse de départ et à partir de celle-ci, l'adresse des registre flag et fifo représente un décalage par rapport à cette adresse. Le registre flag de l'uartO ce trouve donc à l'adresse 0x101F1018.

- Où est ce que on écrit réellement dans quel mémoire ? Quelles sont les adresses où j'écrit ? 

Pour comprendre cela je me suis penché sur le tableau de mappage des adresses que l'on retrouve slide 22 du cours (Lecture1.pdf)

Chaque système à une procédure de mappage des adresses afin d'attribuer différentes plages d'adresses à différentes fonctionnalité de ce dernier. Selon la plage d'adresse utilisé on accède à différentes mémoire physique. Ainsi, les registres de UART sont stockés dans une mémoire reservées aux périphériques , sur ces derniers. Elle est mappé et accéssible graces aux adresses si dessus. 


- Quelles sont les différentes mémoires ? Quels sont leurs roles et qui y accèdent?

Un SoC est une puce qui contient tout ce que un ordinateur aurait besoin pour fonctionner: CPU, mémoire, periphériques, bus de communication et FPGA.
La mémoire du SoC est diviser en deux la RAM et la ROM.
La ram correspond aux mémoire vive et volatile. C'est à dire que cette derniere est vidé lorsque l'on arrete le systeme. Dans la ram on retrouve l'OCM qui est interne au processeur et sert à stoquer des données critiques et d'amorçage comme le firmware ou le bootloader au démarage. Elle est très rapide et ne contient que quelque Ko-Mo. On retrouve aussi parmis la RAM, la DDR est externe est connecté via bus mémoire. Elle permet de stocker le code et les applications en cours d'execution et les données volumineuse. Cette dernière est plus lente que l'OCM mais plus grande (Go)

Ensuite, on retrouve la mémoire ROM ou flash qui est non volatile. Ici, c'est une mémoire Quad-SPI qui est utilisé. Il existe d'autres mémoire ROM comme la mémoire NAND qui est moins cher mais moins rapide. C'est deux mémoires flash sont plus lente que les mémoires RAM mais on de plus grande capacités. Elles permettent de stoquer l'OS, le firmware et les fichiers utilisateurs. C'est dernière sont connectées au serveur grace aux plages d'adresse SMC qui permettent de connecter les mémoires externes au serveur.

Dans le systeme de mappage des adresses on retrouve aussi des plages d'adresses pour la partie PL qui correspond à la partie FPGA d'un SoC. La FPGA est une puce qui permet de reprogrammer du matériel. On retrouve aussi la plage IOP qui correspond aux adresses des périphériques (comme UART), la plage des registre SLCR qui permettent de configurer les composants du SoC. Par exemple de modifier la fréquence du CPU, des bus, d'activer ou de désactiver des composants. Et la plage d'adresse des registres PS qui controle et configure le processeur, les périphériques et la mémoire. On y retrouve les CR pour configurer le processeur, IR pour gérer les intéruptions, les SR qui connaissent l'état du processeur et le MMR qui gère les interactions entre l'espace d'adressage du processeur et l'espace physique de la mémoire.

- Qu'est ce qui ce passe lorsque j'allume mon ordinateur ? Qu'est ce qui ce lance ? Qui est responssable de quel partie logiciel, matériel ?
Quand on lance une machine le cpu commence par charger le firmware dans l'ocm et l'exécuter. Ce dernier sert à initialiser le matériel en configurant la mémoire, activant les périfériques puis il charge le bootloader toujours dans l'ocm qui lui est responsable de charger le systeme d'exploitation ou l'application principale et de la démarrer en donnant le controle à celui ci via un saut à l'addresse où il commence son execution. Le bootloader peut etre contenu dans le firmware. Pour nous tout cela est dans kernel.elf qui est lancé lorsque l'on lance notre machine émulée avec QEMU. Tous est directement chargé en mémoire grace au kernel.ld. Il n'y a donc pas de chargement en mémoire effectué. La mémoire est configurée par les fichiers .s et le saut est effectué dans startup.s avec : 
    ldr r3,=_start
    blx r3 


- Comment fonctionnent les intéruptions ? Pourquoi les utilisers ? 
Utiliser les interruptions permet que plutot que le cpu soit en attente de recevoir des données et qu'il tourne en permanence. Il s'active pour gerer l'envoi des données quand il reçoit une interruption.

Quand le CPU reçoit une exception, il connait l'adresse à laquelle il doit aller en fonction de l'exception reçue. Si c'est une exception d'interruption IRQ il execute l'instruction à l'adresse 0x18. Le fichier exception.s determine les différentes instructions executées en fonction de l'exception. On voit que l'instruction à l'adresse 0x18 est : 
    ldr pc, irq_handler_addr
Il va donc sauter à l'adresse irq_handler_addr qui correspond à l'adresse de la fonction _isr_handler. Et va donc executer cette fonction pour gérer l'interruption. Pour executer l'interruption et revenir au même état que avant ça detection il faut donc le sauvegarder. 
Pour executer l'exception le cpu doit desactiver ces intéruption pour ne pas en avoir de nouvelles pendant qu'il traite l'interruption en cours. 
Le processeur sauvearde automatiquement l'adresse du PC dans le link reister et le CPSR dans SPSR_irq et passe en mode interruption. Mais l'instruction ayant commencé l'adresse du pc est l'adresse de l'instruction suivante. Il faut donc enlevé 4 octets à LR avant d'enregistrer l'état des registre, d'appeler la fonction isr() qui traite l'interruption. 

Avant d'avoir une interruption qui arrive au cpu elle doit etre générée par le port uart et transmise vers le VIC puis vers le CPU.

- Comment l'envoie des données peut generer une interruption Uart ?

En uart il y a deux sortes d'interruptions: RX qui concerne l'interruption à la reception (en fonction de l'état de la fifo de reception, s'active lorsque suffisament de données sont reçu ) et Tx sur la transmittion (en fonction de l'état de la fifo de transmission, s'active quand suffisament d'espace est libéré dans la fifo de transfission).
Pour générer et gerer une interruption UART il existe different registres:
    UARTIMSC qui permet d'activer et de désactiver des interruptions
    UARTRIS qui permet de savoir si une interruption à eu lieu ou non est et en attente
    UARTMIS pour savoir lesquelles sont réellement activent c'est à dire en attente et activées

- comment celle ci sont envoyées au VIC ? 
Le vic vient vérifier le status de ses lignes d'interruptions si un des bits est à 1 cela signifie qu'une interruption a eu lieu. Si cette interruption est desactivé on l'ignore sinon on l'a transmet au CPU.
Pour le vic chaque périphérique à un numéro d'intéruption qui permet de vérifier l'état de cette dernière sur chaque registre.
Par exemple si le 12 eme bit du registre VICIRQSTATUS est 1 cela signifie qu'une interruption à été transmise depuis le port UART0 et est active. Donc qu'elle doit être transmise au CPU


- Pour l'instant je ne comprends pas comment les interruptions sont envoyées entre les trois et quand est ce qu'on doit activer ou descactiver les interruptions 

Le transfert d'interruption entre l'uart et le vic ce fait via une ligne IRQ dédiée par un signal éléctrique. Pareil entre le vic et le cpu. Si l'uart est configuré pour générer une interruption à la reception. Alors dès qu'une donnée est reçu il génère une interruption vers le VIC. Si le vic est configuré avec les interruptions correspondantes activées alors transmet automatiquement l'interruption au cpu.
Le cpu va alors effectuer la fonction de gestion de cette interruption. Nous on veut que cette dernière permette d'afficher le caractere tapé à l'écran. La fonction callback doit donc correspondre à l'execution de uart_receive(UART0, &c);
uart_send(UART0, c);

Vu que la reception crait une exception l'ecriture dans le terminal va faire sortir de la fonction _start. Le debut en sera quand même executé pour initialisé l'uart et son relevé d'exception.


- quand activer/desactiver les interruptions pour le cpu et le vic?

Au lancement, je commence par appeler vic_setup_irqs() qui initialise le matériel pour qu'il puisse recevoir des interruptions et initialise les handlers pour dire que aucune intéruption n'est encore traitée. Nettoy les intéruptions actives et désactive les interruptions pour le vic. 
Ensuite j'initialise les UARTs et active les interruptions rx pour UART0. J'active les intéruptions pour le vic en associant au numero d'interruption de uartO la fonction de traitement de cette interruption, qui fait ici l'affichage. Ensuite, j'active les interruptions CPU. J'ai laissé la boucle for mais son contenu ne sera pas executé car à la reception l'exception sera levé. 

- code pas testé puis voir gestion carractères spéciaux quand code de base fonctionnel
probleme la deuxième interruption ne s'envoit pas je reste en attente.

Le problème venais que je m'étais le bit de VICINTCLEAR à 1 en penssant que cela permetait de remettais le bit du VICSTATUS à 0 alors qu'il remet celui du VICINTENABLE et empeche donc les nouvelles intéruptions. Aussi je met le bit de UART_ICR à 1 ce qui remet bien le bit de UARTMIS à 0 mais n'est pas indispenssable car quand la fifo est vide il est remis à 0 automatiquement. 

- Reconnaissance/Traitement des caractère scpéciaux 
Comme je recupère sur 32 bits les caractères spéciaux comme les caracères accentués sont déjà transmis. C'est dernier étant encodés sur 2 octets si j'utilisais des uint8_ une intéruption ne pourrait pas permettre de les transmettre. Pour ce qui est des flèches,  
elles sont encodées sur 3 octets avec un read sur 32 bits je reupère aussi tout en une seule lecture.
Ces commandes sont directement interprété par notre terminal mais ce n'est pas le cas pour toutes. C'est le cas des Ctrl+lettre.
Sans interprétation si je fais une combinaison de touche comme Ctrl+C rien ne s'affiche car je me contente de transmettre l'octet correspondant au terminal (0x03) qui n'affiche rien. J'ai donc ajouté des inteprétations pour que en fonction de la touche sur laquelle j'appuis j'ai quelque chose qui s'affiche. Je peux par exemple effacer les caractères précédents, sauter une ligne et effacer le contenu du terminal, afficher ou rendre invisible le cursseur. 

Modification: 
Certains caractères et combinaisons de touches on un code aski qui est envoyé à la machine via l'uart quand on les réalises. C'est le cas des lettres et des combinaisons tel que Ctrl + lettre. Cependant, certains code ne sont pas directement interprétes par le terminal. Il faut donc créer une interprétation lors de la leture par la machine avant de transmettre au terminal. Par exemple quand je reçoit 0x03 qui est le code Aski de Ctrl + C j'envoie au terminal \033[H\033[J afin qu'il efface tout le contenu de ce dernier. Aussi certaines touches sont représentées par des combinaisons sur plusieurs bits. Il faut donc lire chaque bit un a un et transmettre l'entiereté de la combinaison pour que le terminal sache ce qu'on lui demande. C'est le cas des flèches. Pour bouger le curseur vers la gauche on reçoit \033 puis [ et enfin D et on transmet \x1B[D. Même si on lit des uint_32 = 4 octets on est en réalité obligé de lire en trois fois car avec uart les bits sont transmit un a un.

Maintenant avec l'execution de ces interpretations la durée de l'execution est plus longue et aucune autre intéruption ne peut être lancée donc il y a un risque de perdre des octets. Il faut donc avoir une fonction de traitement de l'intéruption plus courte en s'occupant uniquement du device qui génère l'intéruption et en laissant l'execution de l'interpretation et l'envoi vers le terminal dans la boucle principal et donc en dehors du traitement de l'intéruption. 
On peut alors stoqué les données envoyées par le device au moment de l'intéruption et les récupérer dans la boucle principale.
Pour cela, il faut soit utiliser une liste mais pour laquelle on ne peut pas lire et ecrire en même temps. Et où il faudra donc bloquer les intéruption au moment de la lecture dans la boucle principale. 
Soit utiliser un buffer circulaire pour lequel on peut avoir le double accès. Si le prochain endroit où écrire égal l'endroit ou lire alors le buffer est plein. Et si l'endroit où lire et écrire sont égaux alors il est vide. 


Ajouter des commandes shell si le reste est fini 
// on va ecrire dans un buffer general avec interuption à l'écriture (revoir cela dans slide de cours )



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


// si je fais for (;;) {
    
    process_buffer();
    core_disable_irqs();
    if (ring_is_empty()) {
      //core_enable_irqs(); 
      core_halt();
    }
    
  } les interuptions ne sont jamais autorisés donc start s'execute en boucle 
  et dans process_buffer je lit la ring qui contient "" puis l'interprete 
  et comme qd dans mon intepre le else envoit le caractère ici vide car jamais lu sur l'uart vu que uart_isr n'est pas appelé. En fait mon core_halt ne réactivait pas les intéruptions. j'ai donc ajouté core_enable_irqs(); à l'intérieur

  