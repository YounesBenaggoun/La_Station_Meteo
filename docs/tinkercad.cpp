const int Button_pin = 2;
bool currentButtonState = 0;
bool lastButtonState = 0;

const int red_led = 5;
const int green_led = 4;
bool green_led_state = 0;

int temperaturePin = A1;
int humidityPin = A0;


float humidity = 0;
float temperature = 0;
float finaleTemperature = 0;


bool degreeMode = 0;

unsigned long t0 = 0;



void setup()
{
  Serial.begin(115200);
  Serial.println("READY");

  pinMode(Button_pin, INPUT_PULLUP);
  pinMode(temperaturePin,INPUT);
  pinMode(humidityPin, INPUT);
}

void loop()
{

  humidity = analogRead(humidityPin);
  finaleTemperature = getTemp();

  Serial.println(humidity);


}


float fahrenheitToCelsius(float fahren){
  return (fahren - 32) / 1.8;
}

float getTemp(){
  float calculatedTemp = 0;

  temperature = analogRead(temperaturePin);

  currentButtonState = digitalRead(Button_pin);

  if(currentButtonState != lastButtonState){
         lastButtonState = currentButtonState;
        if(currentButtonState == 0){
              //Serial.println("keyDown ");
        }else{
              //Serial.println("KeyUp ");
          if(green_led_state){
            digitalWrite(green_led, HIGH);
            digitalWrite(red_led, LOW);
            green_led_state = 0;
            degreeMode = 1;

            //Serial.println(finaleTemperature);
          }else{
            digitalWrite(green_led, LOW);
            digitalWrite(red_led, HIGH);
            green_led_state = 1;
            degreeMode = 0;
          } 
        }
      }

  if(degreeMode){
        calculatedTemp = fahrenheitToCelsius(temperature);
  }else{
        calculatedTemp = temperature;
  }
  return calculatedTemp;

}