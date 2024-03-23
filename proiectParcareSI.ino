#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

// declarăm pinii digitali 
const int greenLedPin = 13;
const int redLedPin = 12;
const int trigPinIn = 11;
const int echoPinIn = 10;
const int servoPin = 9;
const int trigPinOut = 7;
const int echoPinOut = 6;

LiquidCrystal_I2C lcd(0x27, 16, 2); // declarăm o variabilă de tip ecran LCD cu 16 celule și 2 rânduri
Servo servo;  // declarăm o variabilă de tip Servo

int parkingSpots = 5; // numărul de locuri de parcare prestabilit
float durationIn, distanceIn; // variabile utilizate pentru a colecta măsuratorile de la senzorii ultrasonici
float durationOut, distanceOut;
int simultan = 0; // flag folosit pentru a indica prezența unei situații când două mașini doresc utilizarea barierei simultan

float previousDistance = 0; // variabilă folosită la verificarea îndepărtării unei mașini față de senzor, ce reține măsurătoarea anterioară
int movementsCounter = 5; // contor ce indică numărul de "îndepărtări" ale mașinii față de senzor
float thresholdDistance = 1; // distanță în cm ce reprezintă pragul minim de deplasare a unei mașini pentru ca ea să fie considerată o îndepărtare

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

    // afișăm pe ecranul LCD numărul de locuri libere
    lcd.clear();
    lcd.print("Locuri neocupate");
    lcd.setCursor(7,1);
    lcd.print(parkingSpots);

    // senzorul de la intrarea în parcare ia măsurători
    digitalWrite(trigPinIn, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPinIn, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPinIn, LOW);

    durationIn = pulseIn(echoPinIn, HIGH);
    distanceIn = (durationIn*.0343)/2;
    Serial.print("Distance IN: ");
    Serial.println(distanceIn);

    // senzorul de la ieșirea din parcare ia măsurători
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
    if(distanceIn < 10 || distanceOut < 10)
    {
      if(distanceOut < 10) // tratăm cazul în care o mașină dorește să iasă din parcare
      {
        int time = 10;
        while (time < 700)  // timp de 700ms verificăm dacă pe partea cealaltă nu se dorește simultan intrarea în parcare
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

          if (distanceIn < 10)  // dacă există în același timp o mașină pe partea opusă a barierei,
          {
            // atunci vom începe secvența de verificare a îndepărtării mașinii de la intrare, calculând distanța la care se află
            digitalWrite(trigPinIn, LOW);
            delayMicroseconds(2);
            digitalWrite(trigPinIn, HIGH);
            delayMicroseconds(10);
            digitalWrite(trigPinIn, LOW);

            durationIn = pulseIn(echoPinIn, HIGH);
            distanceIn = (durationIn*.0343)/2;
            Serial.print("Distance IN: ");
            Serial.println(distanceIn);

            previousDistance = distanceIn;
            movementsCounter = 5;
            while (movementsCounter != 0) // cât timp mașina este în curs de îndepărtare,
            {
              // vom afișa un mesaj specific pe ecranul LCD
              lcd.clear();
              lcd.print("Va rugam dati cu");
              lcd.setCursor(4, 1);
              lcd.print("spatele!");

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

              if (distanceIn - previousDistance > thresholdDistance) {  // dacă deplasamentul este consistent înseamnă că mașina se îndepărtează cu adevărat
                movementsCounter--;
                previousDistance = distanceIn; // distanța curentă devine noul etalon pentru viitoarea măsurătoare
              }
              delay(400); // verificăm deplasamentul la fiecare 400ms
            }
            // când îndepărtarea s-a finalizat cu succes, pe ecranul LCD revine mesajul inițial
            lcd.clear();
            lcd.print("Locuri neocupate");
            lcd.setCursor(7, 1);
            lcd.print(parkingSpots);
            break; // situația de solicitare simultană a barierei a fost rezolvată, așa că se continuă cu ridicarea barierei
          }

          time += 10;
          delay(10); // în cele 700ms de verificare a "conflictului" se interoghează senzorul la fiecare 10ms
        }

        // se ridică bariera
        for (int angle = 0; angle <= 90; angle += 1)
        {
          servo.write(angle);
          delay(10);
        }
        
        parkingSpots++; // ieșirea din parcare este în curs deci se incrementează numărul de locuri disponibile

        delay(700);

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

        // se verifică dacă mașina a trecut și de senzorul de la intrare
        distanceIn = 50;
        while(distanceIn >= 10)
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
        }

        // apoi se coboară bariera
        for (int angle = 90; angle >= 0; angle -= 1) 
        {
          servo.write(angle);
          delay(10);
        }

        // începe secvența de verificare a îndepărtării mașinii față de senzorul de la intrare, deoarece el e ultimul pe lângă care trece
        // algoritmul este identic cu cel aplicat în situația solicitării simultane a barierei
        // această verificare e necesară deoarece nu ne dorim ca bariera să se ridice prea rapid, creându-se eroarea cum că mașina care tocmai iasă ar dori de fapt să reintre imediat
        // astfel se generează un delay într-un mod mai dinamic
        digitalWrite(trigPinIn, LOW);
        delayMicroseconds(2);
        digitalWrite(trigPinIn, HIGH);
        delayMicroseconds(10);
        digitalWrite(trigPinIn, LOW);

        durationIn = pulseIn(echoPinIn, HIGH);
        distanceIn = (durationIn*.0343)/2;
        Serial.print("Distance IN: ");
        Serial.println(distanceIn);

        previousDistance = distanceIn;
        movementsCounter = 5;
        while(movementsCounter != 0)
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

          if(distanceIn - previousDistance > thresholdDistance)
          {
            movementsCounter--;
            previousDistance = distanceIn;
          }
          delay(400);
        }
      }
      else if(distanceIn < 10) //tratăm cazul în care se dorește intrarea în parcare
      {
        if(parkingSpots > 0)
        {
          int time = 10;
          while (time < 700)  // timp de 700ms verificăm dacă pe partea cealaltă nu se dorește simultan ieșirea din parcare
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

            if (distanceOut < 10)  // dacă există în același timp o mașină pe partea opusă a barierei,
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

              previousDistance = distanceIn;
              movementsCounter = 5;
              while (movementsCounter != 0) 
              {
                lcd.clear();
                lcd.print("Va rugam dati cu");
                lcd.setCursor(4, 1);
                lcd.print("spatele!");

                digitalWrite(trigPinIn, LOW);
                delayMicroseconds(2);
                digitalWrite(trigPinIn, HIGH);
                delayMicroseconds(10);
                digitalWrite(trigPinIn, LOW);

                durationIn = pulseIn(echoPinIn, HIGH);
                distanceIn = (durationIn*.0343)/2;
                Serial.print("Distance IN: ");
                Serial.println(distanceIn);

                if (distanceIn - previousDistance > thresholdDistance) {
                  movementsCounter--;
                  previousDistance = distanceIn;
                }
                delay(400);
              }
              lcd.clear();
              lcd.print("Locuri neocupate");
              lcd.setCursor(7, 1);
              lcd.print(parkingSpots);
              break; // dacă mașina de la intrare s-a îndepărtat cu succes de barieră înseamnă că putem continua spre ridicarea barierei
            }
            simultan = 0;
            time += 10;
            delay(10);
          }

          // se ridică bariera
          for (int angle = 0; angle <= 90; angle += 1)
          {
            servo.write(angle);
            delay(10);
          }

          // dacă bariera a fost ridicată în urma creării unei situații de "conflict" înseamnă că s-a produs o ieșire, deci incrementăm numărul de locuri libere
          // în caz contrar, înseamnă că s-a produs o intrare, deci decrementăm numărul de locuri libere
          if(simultan) parkingSpots++;
          else parkingSpots--;

          delay(700);

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

          if(simultan) // dacă am avut de a face cu situația de "simultan", atunci așteptăm ca mașina ce a ieșit să treacă și de senzorul de la intrare
          {
            distanceIn = 50;
            while(distanceIn >= 10)
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
            }
          }
          else // altfel, așteptăm ca mașina ce a intrat să treacă și de senzorul de la ieșire
          {
            distanceOut = 50;
            while(distanceOut >= 10)
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
            }
          }

          // se coboară bariera
          for (int angle = 90; angle >= 0; angle -= 1) 
          {
            servo.write(angle);
            delay(10);
          }

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

            previousDistance = distanceIn;
            movementsCounter = 5;
            while(movementsCounter)
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

              if(distanceIn - previousDistance > thresholdDistance)
              {
                movementsCounter--;
                previousDistance = distanceIn;
              }
              delay(400);
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

            previousDistance = distanceOut;
            movementsCounter = 5;
            while(movementsCounter)
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

              if(distanceOut - previousDistance > thresholdDistance)
              {
                movementsCounter--;
                previousDistance = distanceOut;
              }
              delay(400);
            }
          }
        }
      }
    }
    delay(100); // senzorii calculează distanțe la fiecare 100ms pentru a capta într-un timp cât mai apropiat de cel real o solicitare de ridicare a barierei
  }
}
