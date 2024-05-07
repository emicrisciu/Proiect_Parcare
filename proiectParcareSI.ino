#include <Wire.h> // bibliotecă (încorporată deja în Arduino IDE) ce ne permite să utilizăm modulul I2C
#include <LiquidCrystal_I2C.h> // biblioteca conține funcțiile clear, print și setCursor utile pentru a reseta ecranul, afișa un text dorit pe ecran și a muta cursorul ce indică 
                               // începutul unei noi porțiuni de text
                               // link preluare bibliotecă: https://robojax.com/learn/arduino/?vid=robojax-LCD1602-I2C
#include <Servo.h> // bibliotecă (încorporată deja în Arduino IDE) ce ne ajută să lucrăm cu un servomotor și conține funcția write ce ne permite să controlam rotația servomotorului,
                   //precum și funcția attach ce ne permite să precizăm pinul de pe placă corespunzător pinului PWM al servomotorului

// declarăm pinii digitali 
const int greenLedPin = 13;
const int redLedPin = 12;
const int trigPinIn = 11;
const int echoPinIn = 10;
const int servoPin = 9;
const int trigPinOut = 7;
const int echoPinOut = 6;

LiquidCrystal_I2C lcd(0x27, 16, 2); // declarăm un obiect de tip ecran LCD cu 2 rânduri a câte 16 celule și adresa 0x27
Servo servo;  // declarăm un obiect de tip Servo

int parkingSpots = 5; // numărul de locuri de parcare prestabilit
float durationIn, distanceIn; // variabile utilizate pentru a colecta măsuratorile de la senzorul ultrasonic de la intrarea in parcare
float durationOut, distanceOut; // variabile utilizate pentru a colecta măsuratorile de la senzorul ultrasonic de la ieșirea din parcare
int simultan = 0; // flag folosit pentru a indica prezența unei situații când două mașini doresc utilizarea barierei simultan

int entryDistance = 7; // când o mașină se află la o distanță mai mică decât acest prag față de senzor, atunci este considerată suficient de aproape de barieră pentru a o deschide
int farThreshold = 10; // când o mașină se află la o distanță mai mare decât acest prag față de senzor, atunci este considerată îndepărtată suficient

// funcția setup, în care se configurează pinii digitali
void setup() {
  pinMode(greenLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  lcd.begin();
  servo.attach(servoPin);
  servo.write(0);
  pinMode(trigPinIn, OUTPUT);
	pinMode(echoPinIn, INPUT);
  pinMode(trigPinOut, OUTPUT);
	pinMode(echoPinOut, INPUT);
	Serial.begin(9600);
}

void loop() {
  while(1)
  {
    // controlăm LED-urile
    if(parkingSpots > 0) // atunci când parcarea mai are locuri libere, LED-ul verde va fi aprins, iar cel roșu stins
    {
      digitalWrite(greenLedPin, HIGH);
      digitalWrite(redLedPin, LOW);
    }
    else // atunci când parcarea nu mai are locuri libere, LED-ul roșu va fi aprins, iar cel verde stins
    {
      digitalWrite(greenLedPin, LOW);
      digitalWrite(redLedPin, HIGH);
    }

    // afișăm pe ecranul LCD numărul de locuri libere
    lcd.clear();
    lcd.print("Locuri neocupate");
    lcd.setCursor(7,1);
    lcd.print(parkingSpots);

    // senzorul de la intrarea în parcare ia măsurători
    digitalWrite(trigPinIn, LOW);
    delayMicroseconds(2); // resetăm pinul Trig pe 0 logic pentru 2 microsecunde ca sa ne asiguram ca porneste de pe 0 logic
    digitalWrite(trigPinIn, HIGH);
    delayMicroseconds(10); // setăm pinul Trig pe 1 logic pentru 10 microsecunde pentru ca senzorul să genereze o explozie ultrasonică de 8 cicluri
    digitalWrite(trigPinIn, LOW); // resetăm pinul Trig pe 0 logic, lucru ce duce la setarea imediată a pinului Echo pe 1 logic

    durationIn = pulseIn(echoPinIn, HIGH); // stocăm rezultatul funcției pulseIn ce ne măsoară timpul în care pinul Echo rămâne pe 1 logic
    distanceIn = (durationIn*.0343)/2; // calculăm distanța în cm față de obiectul detectat
    Serial.print("Distance IN: ");
    Serial.println(distanceIn);

    // senzorul de la ieșirea din parcare ia măsurători în mod similar cu cel de la intrare
    digitalWrite(trigPinOut, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPinOut, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPinOut, LOW);

    durationOut = pulseIn(echoPinOut, HIGH);
    distanceOut = (durationOut*.0343)/2;
    Serial.print("Distance OUT: ");
    Serial.println(distanceOut);

    // verificăm dacă se dorește ridicarea barierei
    if(distanceIn < entryDistance || distanceOut < entryDistance)
    {
      if(distanceOut <= entryDistance) // tratăm cazul în care o mașină dorește să iasă din parcare
      {

        // secvență de validare a faptului că într-adevăr se află o mașină la barieră (durează 2 secunde)
        int time = 0;
        bool valid = true;
        while(time < 200)
        {
          digitalWrite(trigPinOut, LOW);
          delayMicroseconds(2);
          digitalWrite(trigPinOut, HIGH);
          delayMicroseconds(10);
          digitalWrite(trigPinOut, LOW);

          durationOut = pulseIn(echoPinOut, HIGH);
          distanceOut = (durationOut*.0343)/2;
          Serial.print("Distance OUT: ");
          Serial.println(distanceOut);

          if(distanceOut > entryDistance)
          {
            valid = false;
            break;
          }

          time += 10;
          delay(100);
        }

        if(valid) // ne punem problema ridicării barierei doar dacă suntem siguri că avem o mașină prezentă în proximitatea ei
        {
          time = 0;
          while (time < 700)  // timp de 1s verificăm dacă pe partea cealaltă nu se dorește simultan intrarea în parcare
          {
            // verificăm distanța calculată de senzorul de la intrarea în parcare
            digitalWrite(trigPinIn, LOW);
            delayMicroseconds(2);
            digitalWrite(trigPinIn, HIGH);
            delayMicroseconds(10);
            digitalWrite(trigPinIn, LOW);

            durationIn = pulseIn(echoPinIn, HIGH);
            distanceIn = (durationIn*.0343)/2;
            Serial.print("Distance IN: ");
            Serial.println(distanceIn);

            if (distanceIn < entryDistance)  // dacă există în același timp o mașină pe partea opusă a barierei,
            {
              // atunci vom începe secvența de verificare a îndepărtării mașinii de la intrare
              digitalWrite(trigPinIn, LOW);
              delayMicroseconds(2);
              digitalWrite(trigPinIn, HIGH);
              delayMicroseconds(10);
              digitalWrite(trigPinIn, LOW);

              durationIn = pulseIn(echoPinIn, HIGH);
              distanceIn = (durationIn*.0343)/2;
              Serial.print("Distance IN: ");
              Serial.println(distanceIn);

              while (distanceIn < farThreshold) // cât timp mașina este în curs de îndepărtare,
              {
                // vom afișa un mesaj specific pe ecranul LCD
                lcd.clear();
                lcd.print("Va rugam dati cu");
                lcd.setCursor(4, 1);
                lcd.print("spatele!");

                delay(200); // verificăm deplasamentul la fiecare 200ms

                // și vom continua cu colectarea distanței curente
                digitalWrite(trigPinIn, LOW);
                delayMicroseconds(2);
                digitalWrite(trigPinIn, HIGH);
                delayMicroseconds(10);
                digitalWrite(trigPinIn, LOW);

                durationIn = pulseIn(echoPinIn, HIGH);
                distanceIn = (durationIn*.0343)/2;
                Serial.print("Distance IN: ");
                Serial.println(distanceIn);
              }
              // când îndepărtarea s-a finalizat cu succes, pe ecranul LCD revine mesajul inițial
              lcd.clear();
              lcd.print("Locuri neocupate");
              lcd.setCursor(7, 1);
              lcd.print(parkingSpots);
              break; // situația de solicitare simultană a barierei a fost rezolvată, așa că se continuă cu ridicarea barierei
            }

            time += 70; // time se incrementează cu 70 la fiecare pas, iar cum condiția din while îl limitează la valoarea 700, rezultă că vom face 10 măsurători 
            delay(100); // iar cele 10 măsurători vor introduce fiecare câte 100ms de întârziere, ceea ce duce la un delay cumulat de 1s
          }

          delay(300); // pentru a avea o mișcare cât mai realistă a barierei, vom mai aștepta un timp scurt până ridicăm bariera, nu o acționăm instantaneu

          // se ridică bariera în mod fluent
          for (int angle = 0; angle <= 90; angle += 1)
          {
            servo.write(angle);
            delay(10);
          }
          
          // se verifică dacă mașina a trecut și de senzorul de la intrare, timp de 4 secunde
          time = 0;
          bool presence = false;
          while(time < 400)
          {
            digitalWrite(trigPinIn, LOW);
            delayMicroseconds(2);
            digitalWrite(trigPinIn, HIGH);
            delayMicroseconds(10);
            digitalWrite(trigPinIn, LOW);

            durationIn = pulseIn(echoPinIn, HIGH);
            distanceIn = (durationIn*.0343)/2;
            Serial.print("Distance IN: ");
            Serial.println(distanceIn);

            if(distanceIn <= entryDistance)
            {
              presence = true;
              break;
            }

            time += 10;
            delay(100);
          }

          if(presence) 
          {
            parkingSpots++; // ieșirea din parcare este în curs deci se incrementează numărul de locuri disponibile

            // începe secvența de verificare a îndepărtării mașinii față de senzorul de la intrare, deoarece el e ultimul pe lângă care trece
            // algoritmul este identic cu cel aplicat în situația solicitării simultane a barierei
            // această verificare e necesară deoarece nu ne dorim ca bariera să se ridice prea rapid, creându-se eroarea cum că mașina care tocmai iasă ar dori de fapt să reintre imediat
            // astfel se generează un delay într-un mod mai dinamic
            time = 0;

            digitalWrite(trigPinIn, LOW);
            delayMicroseconds(2);
            digitalWrite(trigPinIn, HIGH);
            delayMicroseconds(10);
            digitalWrite(trigPinIn, LOW);

            durationIn = pulseIn(echoPinIn, HIGH);
            distanceIn = (durationIn*.0343)/2;
            Serial.print("Distance IN: ");
            Serial.println(distanceIn);

            while (distanceIn < farThreshold || time < 100) // cât timp mașina este în curs de îndepărtare,
            {
              // și vom continua cu colectarea distanței curente
              digitalWrite(trigPinIn, LOW);
              delayMicroseconds(2);
              digitalWrite(trigPinIn, HIGH);
              delayMicroseconds(10);
              digitalWrite(trigPinIn, LOW);

              durationIn = pulseIn(echoPinIn, HIGH);
              distanceIn = (durationIn*.0343)/2;
              Serial.print("Distance IN: ");
              Serial.println(distanceIn);

              time += 10;
              delay(200); // verificăm deplasamentul la fiecare 200ms
            }

            delay(300); // se mai așteaptă puțin, tot din motive de realism
          }
          
          // apoi se coboară bariera
          for (int angle = 90; angle >= 0; angle -= 1) 
          {
            servo.write(angle);
            delay(10);
          }

          // actualizare stare LED-uri
          if(parkingSpots > 0)
          {
            digitalWrite(greenLedPin, HIGH);
            digitalWrite(redLedPin, LOW);
          }
          else
          {
            digitalWrite(greenLedPin, LOW);
            digitalWrite(redLedPin, HIGH);
          }

          // afișăm pe ecranul LCD numărul actualizat de locuri libere
          lcd.clear();
          lcd.print("Locuri neocupate");
          lcd.setCursor(7,1);
          lcd.print(parkingSpots);
        }
        else
        {
          delay(2000);
        }
      }
      else if(distanceIn <= entryDistance) //tratăm cazul în care se dorește intrarea în parcare
      {
        if(parkingSpots > 0)
        {
          int time = 0;
          bool valid = true;
          while(time < 700)  // validare 1s
          {
            digitalWrite(trigPinIn, LOW);
            delayMicroseconds(2);
            digitalWrite(trigPinIn, HIGH);
            delayMicroseconds(10);
            digitalWrite(trigPinIn, LOW);

            durationIn = pulseIn(echoPinIn, HIGH);
            distanceIn = (durationIn*.0343)/2;
            Serial.print("Distance IN: ");
            Serial.println(distanceIn);

            if(distanceIn > entryDistance)
            {
              valid = false;
              break;
            }

            time += 70;
            delay(100);
          }

          if(valid)
          {
            time = 0;
            while (time < 700)  // timp de 1s verificăm dacă pe partea cealaltă nu se dorește simultan ieșirea din parcare
            {
              // situația se tratează similar cu cea întâlnită la cazul ieșirii din parcare 
              digitalWrite(trigPinOut, LOW);
              delayMicroseconds(2);
              digitalWrite(trigPinOut, HIGH);
              delayMicroseconds(10);
              digitalWrite(trigPinOut, LOW);

              durationOut = pulseIn(echoPinOut, HIGH);
              distanceOut = (durationOut*.0343)/2;
              Serial.print("Distance OUT: ");
              Serial.println(distanceOut);

              if (distanceOut < entryDistance)  // dacă există în același timp o mașină pe partea opusă a barierei,
              {
                simultan = 1; // atunci setăm variabila "simultan", deoarece ne aflăm într-un caz mai special
                // gestionând intrarea în parcare, ne dorim să prioritizăm mașina ce dorește să iasă pentru a se elibera astfel un loc
                // vom forța astfel ca mașina ce așteaptă intrarea în parcare să se îndepărteze pentru a-i face loc celei care dorește să iasă

                // începe secvența care verifică îndepărtarea mașinii de la intrare
                digitalWrite(trigPinIn, LOW);
                delayMicroseconds(2);
                digitalWrite(trigPinIn, HIGH);
                delayMicroseconds(10);
                digitalWrite(trigPinIn, LOW);

                durationIn = pulseIn(echoPinIn, HIGH);
                distanceIn = (durationIn*.0343)/2;
                Serial.print("Distance IN: ");
                Serial.println(distanceIn);

                while (distanceIn < farThreshold) 
                {
                  lcd.clear();
                  lcd.print("Va rugam dati cu");
                  lcd.setCursor(4, 1);
                  lcd.print("spatele!");

                  delay(200);

                  digitalWrite(trigPinIn, LOW);
                  delayMicroseconds(2);
                  digitalWrite(trigPinIn, HIGH);
                  delayMicroseconds(10);
                  digitalWrite(trigPinIn, LOW);

                  durationIn = pulseIn(echoPinIn, HIGH);
                  distanceIn = (durationIn*.0343)/2;
                  Serial.print("Distance IN: ");
                  Serial.println(distanceIn);
                }
                lcd.clear();
                lcd.print("Locuri neocupate");
                lcd.setCursor(7, 1);
                lcd.print(parkingSpots);
                break; // dacă mașina de la intrare s-a îndepărtat cu succes de barieră înseamnă că putem continua spre ridicarea barierei
              }
              simultan = 0;
              time += 70;
              delay(100);
            }

            delay(300); // pentru a avea o mișcare cât mai realistă a barierei, vom mai aștepta un timp scurt până ridicăm bariera, nu o acționăm instantaneu

            // se ridică bariera
            for (int angle = 0; angle <= 90; angle += 1)
            {
              servo.write(angle);
              delay(10);
            }

            bool presenceOut = false;

            if(simultan) // dacă am avut de a face cu situația de "simultan", atunci așteptăm ca mașina ce a ieșit să treacă și de senzorul de la intrare
            {
              lcd.clear();
              lcd.print("simultan");
              time = 0;
              presenceOut = false;
              while(time < 400)
              {
                digitalWrite(trigPinIn, LOW);
                delayMicroseconds(2);
                digitalWrite(trigPinIn, HIGH);
                delayMicroseconds(10);
                digitalWrite(trigPinIn, LOW);

                durationIn = pulseIn(echoPinIn, HIGH);
                distanceIn = (durationIn*.0343)/2;
                Serial.print("Distance IN: ");
                Serial.println(distanceIn);

                if(distanceIn <= entryDistance)
                {
                  presenceOut = true;
                  break;
                }

                time += 10;
                delay(100);
              }
            }
            else // altfel, așteptăm ca mașina ce a intrat să treacă și de senzorul de la ieșire
            {
              lcd.clear();
              lcd.print("nesimultan");
              time = 0;
              presenceOut = false;
              while(time < 400)
              {
                digitalWrite(trigPinOut, LOW);
                delayMicroseconds(2);
                digitalWrite(trigPinOut, HIGH);
                delayMicroseconds(10);
                digitalWrite(trigPinOut, LOW);

                durationOut = pulseIn(echoPinOut, HIGH);
                distanceOut = (durationOut*.0343)/2;
                Serial.print("Distance OUT: ");
                Serial.println(distanceOut);

                if(distanceOut <= entryDistance)
                {
                  presenceOut = true;
                  break;
                }

                time += 10;
                delay(100);
              }
            }

            if(presenceOut)
            {
              // dacă bariera a fost ridicată în urma creării unei situații de "conflict" înseamnă că s-a produs o ieșire, deci incrementăm numărul de locuri libere
              // în caz contrar, înseamnă că s-a produs o intrare, deci decrementăm numărul de locuri libere
              if(simultan) parkingSpots++;
              else parkingSpots--;

              // similar cu cazul anterior, pentru a nu se ridica bariera prea rapid la venirea unei eventuale viitoare cereri, vom aștepta îndepărtarea mașinii ce a intrat/ieșit
              if(simultan) // dacă am avut de a face cu situația de "simultan", atunci vom aștepta îndepărtarea mașinii ce a ieșit din parcare
              {
                digitalWrite(trigPinIn, LOW);
                delayMicroseconds(2);
                digitalWrite(trigPinIn, HIGH);
                delayMicroseconds(10);
                digitalWrite(trigPinIn, LOW);

                durationIn = pulseIn(echoPinIn, HIGH);
                distanceIn = (durationIn*.0343)/2;
                Serial.print("Distance IN: ");
                Serial.println(distanceIn);

                time = 0;
                while(distanceIn < farThreshold || time < 700)
                {
                  digitalWrite(trigPinIn, LOW);
                  delayMicroseconds(2);
                  digitalWrite(trigPinIn, HIGH);
                  delayMicroseconds(10);
                  digitalWrite(trigPinIn, LOW);

                  durationIn = pulseIn(echoPinIn, HIGH);
                  distanceIn = (durationIn*.0343)/2;
                  Serial.print("Distance IN: ");
                  Serial.println(distanceIn);

                  time += 70;
                  delay(200);
                }
              }
              else // altfel, vom aștepta îndepărtarea mașinii ce a intrat în parcare
              {
                digitalWrite(trigPinOut, LOW);
                delayMicroseconds(2);
                digitalWrite(trigPinOut, HIGH);
                delayMicroseconds(10);
                digitalWrite(trigPinOut, LOW);

                durationOut = pulseIn(echoPinOut, HIGH);
                distanceOut = (durationOut*.0343)/2;
                Serial.print("Distance OUT: ");
                Serial.println(distanceOut);

                time = 0;
                while(distanceOut < farThreshold || time < 700)
                {
                  digitalWrite(trigPinOut, LOW);
                  delayMicroseconds(2);
                  digitalWrite(trigPinOut, HIGH);
                  delayMicroseconds(10);
                  digitalWrite(trigPinOut, LOW);

                  durationOut = pulseIn(echoPinOut, HIGH);
                  distanceOut = (durationOut*.0343)/2;
                  Serial.print("Distance OUT: ");
                  Serial.println(distanceOut);

                  time += 70;
                  delay(200);
                }
              }

              delay(300); // se mai așteaptă puțin, tot din motive de realism
            }

            // se coboară bariera
            for (int angle = 90; angle >= 0; angle -= 1) 
            {
              servo.write(angle);
              delay(10);
            }

            // actualizare stare LED-uri
            if(parkingSpots > 0)
            {
              digitalWrite(greenLedPin, HIGH);
              digitalWrite(redLedPin, LOW);
            }
            else
            {
              digitalWrite(greenLedPin, LOW);
              digitalWrite(redLedPin, HIGH);
            }

            // afișăm pe ecranul LCD numărul actualizat de locuri libere
            lcd.clear();
            lcd.print("Locuri neocupate");
            lcd.setCursor(7,1);
            lcd.print(parkingSpots);
          }
          else
          {
            // lcd.clear();
            // lcd.setCursor(1,0);
            // lcd.print("Acces nepermis");
            delay(2000);
          }
        }
        else
        {
          lcd.clear();
          lcd.setCursor(1,0);
          lcd.print("Parcarea este");
          lcd.setCursor(5,1);
          lcd.print("plina!");
          delay(2000);
        }
      }
    }
    delay(100); // senzorii calculează distanțe la fiecare 100ms pentru a capta într-un timp cât mai apropiat de cel real o solicitare de ridicare a barierei
  }
}