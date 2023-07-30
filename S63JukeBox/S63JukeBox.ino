


// TODO : OK faire sortir de la boucle waitMillisecond lorsque le mp3 est terminé (par affectation d'une variable).
//        OK MOYEN la boulcle sur la tonlité de décroché ne fonctionne pas : idée : capturer la fin de boucle pour la relancer
//        OK la tonalité de décroché met du temps à venir (probable effet des setRepeatPlayCurrentTrack)
//        OK adapter le volume sonore
//        OK Pouvoir interrompre la tonalité au raccroché
//           Code pour pouvoir influer sur le volume : composer un numéro 80xx 80 pour VO de volume et xx pour le volume à régler. Afficher l'ancien volume et le nouveau, faire entendre la tonalité pour test
//        OK désactiver les traces série
//           Pb quand on raccroche en cours, puis décroché et nouveau numéro. Le premier n'est pas pris...

//#define DEBUG
//#define DEBUG2

#define AFFICHE

#ifdef DEBUG
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTLN2(x,y) Serial.println(x,y)
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINT2(x,y) Serial.print(x,y)
  #define DEBUG_PRINTF(x,y) Serial.printf(x,y)
  #define DEBUG_WAIT() Serial.begin(115200)
#else
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTLN2(x,y)
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINT2(x,y)
  #define DEBUG_PRINTF(x,y)
  #define DEBUG_WAIT()
#endif

#define SORTIE D0
#define NIVEAU 500
#define VOLUME 16

int gvol = VOLUME;



#include <SoftwareSerial.h>
//#define DfMiniMp3Debug Serial
#include <DFMiniMp3.h>
class Mp3Notify; 
// define a handy type using serial and our notify class
//

typedef enum {DECROCHE, RACCROCHE, NUMEROTATION} T_POS;

T_POS tel, prevTel;
bool tonalite = false;
bool enLecture = false;

SoftwareSerial secondarySerial(D7, D6); // RX, TX  -> inversé au soudage...
//DFMiniMp3<SoftwareSerial, Mp3Notify, Mp3ChipMH2024K16SS> mp3(secondarySerial);
typedef DFMiniMp3<SoftwareSerial, Mp3Notify, Mp3ChipMH2024K16SS> DfMp3;
DfMp3 dfmp3(secondarySerial);

class Mp3Notify
{
public:
  static void PrintlnSourceAction(DfMp3_PlaySources source, const char* action)
  {
    if (source & DfMp3_PlaySources_Sd) 
    {
        DEBUG_PRINT("SD Card, ");
    }
    if (source & DfMp3_PlaySources_Usb) 
    {
        DEBUG_PRINT("USB Disk, ");
    }
    if (source & DfMp3_PlaySources_Flash) 
    {
        DEBUG_PRINT("Flash, ");
    }
    DEBUG_PRINTLN(action);
  }
  static void OnError([[maybe_unused]] DfMp3& mp3, uint16_t errorCode)
  {
    // see DfMp3_Error for code meaning
    DEBUG_PRINTLN();
    DEBUG_PRINT("Com Error ");
    DEBUG_PRINTLN(errorCode);
  }
  static void OnPlayFinished([[maybe_unused]] DfMp3& mp3, [[maybe_unused]] DfMp3_PlaySources source, uint16_t track)
  {
    DEBUG_PRINT("Play finished for #");
    DEBUG_PRINTLN(track);
    enLecture = false;  
  }
  static void OnPlaySourceOnline([[maybe_unused]] DfMp3& mp3, DfMp3_PlaySources source)
  {
    PrintlnSourceAction(source, "online");
  }
  static void OnPlaySourceInserted([[maybe_unused]] DfMp3& mp3, DfMp3_PlaySources source)
  {
    PrintlnSourceAction(source, "inserted");
  }
  static void OnPlaySourceRemoved([[maybe_unused]] DfMp3& mp3, DfMp3_PlaySources source)
  {
    PrintlnSourceAction(source, "removed");
  }
};
//// fin DFPlayer ////



#ifdef AFFICHE
// Oled
  #include <Adafruit_GFX.h>
  #include <Adafruit_SSD1306.h>
  Adafruit_SSD1306 display = Adafruit_SSD1306();
  #define NOM_PROJ "S63 Revival"

#endif

// Un raccroché dure au moins 200 ms
#define DUREE_RACCROCHE 1000
// la numérotation est à 67ms haut et 33 ms bas (je prends de la marge)
#define DUREE_NUMHIGH 50
#define DUREE_NUMLOW 20

#define NUM_MAX_LEN 4
#define NUM_MINI 1950
#define NUM_MAXI 2005

// durée anti-rebond
#define DUREE_REBOND 5

int duree = 0, t, prevT; // durée depuis le dernier changement d'état
//int state = 0, prevState = 0;  // l'état du téléphone
int ligne = 0;  // l'état courant de la ligne
int prevLigne = 0;
int nbImpulsion = 0;
int nbChiffre = 0;
char numero[NUM_MAX_LEN + 1];



void setup() {
  // put your setup code here, to run once:
  DEBUG_WAIT();
  pinMode(A0, INPUT);
  pinMode(SORTIE, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  duree = 0;
  t = 0;
  prevT = 0;
  tel = DECROCHE;
  prevLigne = (analogRead(A0) > NIVEAU);

  numero[0] = '\0';

#ifdef AFFICHE
  // Oled
   // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  // init done
  display.display();

  delay(100);
  display.clearDisplay();
  display.display();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print(NOM_PROJ);
  display.setCursor(0,10);
  display.print("Initialisation...");
  display.display();

  
#endif


  dfmp3.begin();
  uint16_t version = dfmp3.getSoftwareVersion();
  DEBUG_PRINT("### version ");
  DEBUG_PRINTLN(version);
  dfmp3.reset();
  dfmp3.loop();
  uint16_t volume = dfmp3.getVolume();
  DEBUG_PRINT("volume ");
  DEBUG_PRINTLN(volume);
  dfmp3.setVolume(gvol);
  
  uint16_t count = dfmp3.getTotalTrackCount(DfMp3_PlaySource_Sd);
  DEBUG_PRINT("files ");
  DEBUG_PRINTLN(count);
  dfmp3.setRepeatPlayCurrentTrack(false);
  display.setCursor(0,20);
  display.print("Audio: OK");
  display.display();

}

void waitMilliseconds(uint16_t msWait)
{
  uint32_t start = millis();
  
  while ((millis() - start) < msWait)
  {
    // calling dfmp3.loop() periodically allows for notifications 
    // to be handled without interrupts
    dfmp3.loop(); 
    delay(1);
  }
}

void setVol(int v) {
  int vol = v - 8000;
  uint16_t volume = dfmp3.getVolume();
  DEBUG_PRINT("volume ");
  DEBUG_PRINTLN(volume);
  if (vol == 0) {
    // affichage du volume
  } 
  else {
    dfmp3.setVolume(vol);
    gvol = vol;
  }


#ifdef AFFICHE
  char buf[80];
  sprintf(buf, "volume %d -> %d", volume, vol);
  display.setCursor(0,20);
  display.clearDisplay();
  display.print(buf);
  display.display();
#endif   
  tonalite = true;
  dfmp3.playMp3FolderTrack(9998); // sd:/mp3/9998.mp3 invitation à numéoter
  dfmp3.loop();
}
void raccroche() {
  nbChiffre = 0;
  numero[nbChiffre] = '\0';
  DEBUG_PRINTLN("Réinitialisation du numéro");
  dfmp3.setVolume(1);
  dfmp3.reset();
  //dfmp3.setVolume(VOLUME);   Fait un ploc sur le HP
  dfmp3.loop();
  dfmp3.setVolume(gvol);
  DfMp3_Status stat = dfmp3.getStatus();
  DEBUG_PRINT("Statut :"); DEBUG_PRINT(stat.source); DEBUG_PRINT(" - "); DEBUG_PRINT(stat.state); DEBUG_PRINT(" lecture "); DEBUG_PRINTLN(enLecture);
  
  // arrêt de la diffusion via le reset(), remise en paybackmode normal

  //dfmp3.setRepeatPlayCurrentTrack(false);

  // affichage
#ifdef AFFICHE
  display.setCursor(0,0);
  display.print(NOM_PROJ);
  display.setCursor(0,10);
  display.clearDisplay();
  display.print("Raccroche");
  display.display();
#endif
}

void loop() {
#ifdef DEBUG2
  DEBUG_PRINTLN(analogRead(A0));
#endif
  ligne = (analogRead(A0) > NIVEAU);    // raccroché ou impulsion de numérotation = 1, décroché = 0
#ifdef DEBUG2
  DEBUG_PRINT("ANALOGREAD : "); DEBUG_PRINTLN(analogRead(A0)); //la trace empêche la bonne détection des numéros
#endif
  t = millis();
  duree = t - prevT;

#ifdef DEBUG2
  DEBUG_PRINTLN("Ligne\tprevLi\tduree\ttel ");
  Serial.printf("%d\t%d\t%d\t%d\n", ligne, prevLigne, duree, tel);  
  Serial.flush();
#endif

  // anti-rebond
  if ((ligne != prevLigne) && (duree <= DUREE_REBOND)) {
    //DEBUG_PRINTLN("Anti rebond");
    return;
  }
   
  // Affichage de l'état de la ligne sur la led intégrée
  if (ligne) {
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);  // allumé
  }

  
  // si pas de changmenet d'état de la ligne, pas la peine de continuer
  if ((ligne) && (prevLigne) && ((tel == RACCROCHE) || (duree > DUREE_RACCROCHE))) {
    prevLigne = ligne;
    prevT = t;
    tel = RACCROCHE;
    DEBUG_PRINTLN("Combiné raccroché");
    enLecture = false;
    raccroche();
#ifdef AFFICHE
    display.setCursor(0,10);
    display.clearDisplay();
    display.print("Raccroche");
    display.display();
#endif
    return;
  }

  // Détection de l'inter train de pulsations
  if ((!ligne) && (duree > DUREE_RACCROCHE) && (tel == NUMEROTATION)) {
    DEBUG_PRINTLN("fin du train de pulsations...");
    tel = DECROCHE;

    // enregistrement du numéro composé
    if (nbImpulsion == 10) nbImpulsion = 0;
    numero[nbChiffre++] = '0' + nbImpulsion;
    numero[nbChiffre] = '\0';
    DEBUG_PRINT("Numéro composé : "); DEBUG_PRINTLN(numero);
#ifdef AFFICHE
    display.setCursor(0,20);
    display.clearDisplay();
    display.print("numero ");
    display.print(numero);
    display.display();
#endif
    nbImpulsion = 0;
    prevLigne = ligne;
    // TESTER si on a atteint le nombre de chiffres voulu
    if (nbChiffre == NUM_MAX_LEN) {
      int inum = atoi(numero);
      if ((inum >=8000) && (inum <=8025))
        setVol(inum);
      else {
        DEBUG_PRINT("Lecture mp3 du numéro "); DEBUG_PRINTLN(inum);
        tonalite = false;
        if (inum < NUM_MINI || inum > NUM_MAXI) {
          DEBUG_PRINTLN("Lecture mp3 occupé");
          dfmp3.playMp3FolderTrack(9996); // sd:/mp3/9996.mp3  - 3,5 secondes
          waitMilliseconds(3500);  // 3,5 secondes
          raccroche();
          } else {
          dfmp3.playMp3FolderTrack(9997); // sd:/mp3/9997.mp3 acheminement 2,5 s et sonnerie (7 secondes et quelques)
          waitMilliseconds(5000);  // 7 secondes
          
          DEBUG_PRINTLN("Lecture mp3 numéroté");
          dfmp3.playMp3FolderTrack(inum); // sd:/mp3/xxxx.mp3
          dfmp3.loop();
          enLecture = true;
#ifdef AFFICHE
          display.setCursor(0,10);
          display.clearDisplay();
          display.print("numero ");
          display.print(inum);
          display.setCursor(0,20);
          display.print("lecture en cours...");
          display.display();
#endif        
          }
        }
      }    // nbChiffre == NUM_MAX_LEN
    }
 
  // front montant
  if ((ligne) && (!prevLigne)) {
    DEBUG_PRINTLN("Front Montant");
    if (duree > DUREE_NUMHIGH) {
      if (tel != NUMEROTATION && !enLecture) {   // il y a des faux positifs avec la lecture mp3... On laisse se dérouler le mp3 final
        tel = NUMEROTATION;     // Probable début de numérotation
        DEBUG_PRINTLN("Numérotation en cours");
        // arrêt de la tonalité si nécessaire
        //dfmp3.stop();   //PVG pour détection d'une impulsion
        //dfmp3.loop();
      }
      prevLigne = ligne;
      prevT = t;
    }  else {
      return;
    }
  }   // fin du test front montant

  // Front descendant
  if ((!ligne) && (prevLigne) && (!enLecture)) {
    DEBUG_PRINTLN("Front descendant");
    if (tel == NUMEROTATION) {
      nbImpulsion++;
      prevLigne = ligne;
      prevT = t;
      DEBUG_PRINT("Nb Impulsions : "); DEBUG_PRINTLN(nbImpulsion);
    } else {
      tel = DECROCHE;
      DEBUG_PRINTLN("Décroché...tonalité");
      tonalite = true;
      dfmp3.playMp3FolderTrack(9998); // sd:/mp3/9998.mp3 invitation à numéoter
      dfmp3.loop();
#ifdef AFFICHE
    display.setCursor(0,10);
    display.clearDisplay();
    display.print("Decroche");
    display.display();
#endif
      prevLigne = ligne,
      prevT = t;
    }
  }   // fin du test du front descendant
  // le mp3loop prend peut être du temps et limite la détection d'impulsion...
  if (nbImpulsion == 0) dfmp3.loop();
// ____       _   _     _   _         _________
//     !_____! !_! !___! !_! !_______!
// 0         1         2         3
// 0123456789012345678901234567890123456789


}
