#include <ctime>
#include <fstream>
#include <iostream>
#include <string>

#include "csv.h"
#include "pdfgen.c"
#include "sha256.cpp"

using namespace std;

struct Car
{
    string plateNumber;
    string brand;
    string model;
    int year;
    string color;
    double pricePerDay;
    time_t startDate;
    time_t endDate;
};

struct Client
{
    int ID;
    string firstName;
    string lastName;
    string password;
    string phone;
    string email;
    int nbReservation;
    Car *cars;
    bool admin;
};

void TimeToString(char DateText[], int TextSize, time_t t)
{
    tm *time = localtime(&t);
    strftime(DateText, TextSize, "%d-%m-%Y", time);
}

time_t StringToTime(const char date[])
{
    tm time = {};

    int result = sscanf(date, "%d-%d-%d", &time.tm_mday, &time.tm_mon,
                        &time.tm_year); // return number of arguments that it
                                        // assigned their values.
    if (result != 3)
    {
        cout << "invalid date format, expected: D-M-Y\n";
        return -1;
    }

    time.tm_year -=
        1900;         // mktime expects how many years have passed since 1900
    time.tm_mon -= 1; // month indexing starts from 0

    return mktime(&time); // returns nb of seconds passed since 1970-jan-1 till
                          // the input date
}

time_t TimeOfTodayStart()
{
    time_t current_time = time(NULL);
    tm *local_time = localtime(&current_time);
    local_time->tm_hour = 0;
    local_time->tm_min = 0;
    local_time->tm_sec = 0;
    return mktime(local_time);
}

time_t inputTime(bool isStartDate)
{
    time_t time = 0;
    string date;

    do
    {
        date = "";
        cout << "Enter " << (isStartDate ? "start" : "end")
             << " date (day-month-year): ";
        getline(cin, date);
        time = StringToTime(date.c_str());
    } while (time <= 0);

    return time;
}

void writeClientToFile(ofstream &os, Client client)
{
    os << "\n";
    os << client.ID << "," << client.firstName << "," << client.lastName << ","
       << client.password << "," << client.email << "," << client.phone << ","
       << (client.admin ? "true" : "false");
}

void writeClientsToFile(Client *clients, int size)
{
    ofstream os("clients.csv");
    os << "ID,fName,lName,Pass,Email,phoneNb,admin";
    for (int i = 0; i < size; i++)
        writeClientToFile(os, clients[i]);
    os.close();
}

void addClientToFile(Client client)
{
    ofstream os("clients.csv", ios::app);
    writeClientToFile(os, client);
    os.close();
}

void clients_add(Client *&clients, int &size, Client client)
{
    Client *newClients = new Client[size + 1];
    for (int i = 0; i < size; i++)
        newClients[i] = clients[i];
    newClients[size] = client;

    if (size != 0)
        delete[] clients;

    clients = newClients;
    size += 1;
}

Client *clients_get(Client *clients, int size, int ID)
{
    for (int i = 0; i < size; i++)
        if (clients[i].ID == ID)
            return &clients[i];
    return NULL;
}

void cars_add(Car *&cars, int &size, Car car)
{
    Car *newCars = new Car[size + 1];
    for (int i = 0; i < size; i++)
        newCars[i] = cars[i];
    newCars[size] = car;

    if (size != 0)
        delete[] cars;

    cars = newCars;
    size += 1;
}

Car *cars_get(Car *cars, int size, string plateNumber)
{
    for (int i = 0; i < size; i++)
        if (cars[i].plateNumber == plateNumber)
            return &cars[i];
    return NULL;
}

Car *cars_get_client(Client *clients, int size, string plateNumber, int &ID)
{
    for (int i = 0; i < size; i++)
    {
        Client client = clients[i];
        for (int j = 0; j < client.nbReservation; j++)
        {
            if (client.cars->plateNumber == plateNumber)
            {
                ID = client.ID;
                return client.cars;
            }
        }
    }
    return NULL;
}

void cars_remove(Car *&cars, int &size, string plateNumber)
{
    if (cars_get(cars, size, plateNumber) == NULL)
        return;

    if (size == 1)
    {
        delete[] cars;
        size = 0;
        cars = NULL;
        return;
    }

    Car *newCars = new Car[size - 1];

    int carIndex = 0;
    for (int i = 0; i < size; i++)
        if (cars[i].plateNumber != plateNumber)
            newCars[carIndex++] = cars[i];

    delete[] cars;
    cars = newCars;
    size--;
}

bool validatePassword(string pass)
{
    if (pass.length() < 8)
        return false;

    bool hasNum = false, hasLetter = false, hasSpecial = false;
    pass = pass.c_str();

    for (int i = 0; i < pass.length(); i++)
    {
        if (isdigit(pass[i]))
            hasNum = true;
        else if (isalpha(pass[i]))
            hasLetter = true;
        else
            hasSpecial = true;
    }

    return hasNum && hasLetter && hasSpecial;
}

bool validateEmail(string email)
{
    if (email.length() == 0)
        return false;

    if (!isalpha(email[0])) // email should start by an alpha
        return false;

    int atIndex = -1;

    for (int i = 0; i < email.length(); i++)
    {
        if (email[i] == ' ')
            return false;
        if (email[i] == '@')
            atIndex = i;
    }

    if (atIndex <= 0 || atIndex == email.length() - 1)
        return false;

    for (int i = 0; i < atIndex; i++)
        if (email[i] == '.')
            return false;

    bool hasDot = false;
    for (int i = atIndex + 1; i < email.length() - 1; i++)
        if (email[i] == '.')
        {
            if (i == atIndex + 1) // dot should not be the first thing after @
                return false;
            hasDot = true; // should have at least 1 dot after @
            break;
        }

    return hasDot;
}

bool validatePhoneNumber(string number)
{
    if (number.length() != 9)
        return false;

    if (number[2] != '-')
        return false;

    for (int i = 0; i < number.length(); i++)
        if (i != 2 && !isdigit(number[i]))
            return false;

    return true;
}

int client_getNewID(Client *clients, int size)
{
    if (size == 0)
        return 0;
    return clients[size - 1].ID + 1;
}

bool emailAlreadyExists(Client *clients, int clientsCount, string email)
{
    for (int i = 0; i < clientsCount; i++)
        if (clients[i].email == email)
            return true;
    return false;
}

bool phoneNumberAlreadyExists(Client *clients, int clientsCount, string ph)
{
    for (int i = 0; i < clientsCount; i++)
        if (clients[i].phone == ph)
            return true;
    return false;
}

Client inputClient(Client *clients, int clientsCount)
{
    Client client = {};

    cout << "Enter your fisrt name: ";
    getline(cin, client.firstName);

    cout << "Enter your last name: ";
    getline(cin, client.lastName);

    do
    {
        cout << "Enter your email: ";
        getline(cin, client.email);

        if (!validateEmail(client.email))
        {
            cout << "invalid email format, please try "
                    "another one\n";
            continue;
        }

        if (emailAlreadyExists(clients, clientsCount, client.email))
        {
            cout << "account with such email already exists, please try "
                    "another one\n";
            continue;
        }

        break;
    } while (true);

    do
    {
        cout << "Enter your password: ";
        getline(cin, client.password);
    } while (!validatePassword(client.password));

    SHA256 sha = SHA256();                      // making an SHA256 instance
    sha.update(client.password);                // updating the code to hash
    uint8_t *digest = sha.digest();             // returning the hashing
    client.password = SHA256::toString(digest); // transforming it to string
    delete[] digest;

    do
    {
        cout << "Enter your phone number: ";
        getline(cin, client.phone);

        if (!validatePhoneNumber(client.phone))
        {
            cout << "invalid phone number format, please try "
                    "another one\n";
            continue;
        }

        if (phoneNumberAlreadyExists(clients, clientsCount, client.phone))
        {
            cout << "account with such phone number already exists, please try "
                    "another one\n";
            continue;
        }

        break;
    } while (true);

    return client;
}

Car inputCar()
{
    Car car = {};

    car.startDate = -1;
    car.endDate = -1;

    cout << "Enter the plate number of the car: ";
    getline(cin, car.plateNumber);

    cout << "Enter the brand of the car: ";
    getline(cin, car.brand);

    cout << "Enter the year of production: ";
    cin >> car.year;

    cin.ignore();

    cout << "Enter the model of the car: ";
    getline(cin, car.model);

    cout << "Enter the daily price for rent: ";
    cin >> car.pricePerDay;

    cin.ignore();

    cout << "Enter the color of the car: ";
    getline(cin, car.color);

    return car;
}

enum
{
    EXIT,
    LIST_CARS,
    LIST_RENTED_CARS,
    RENT_CAR,
    CANCEL_RENT,
    MODIFY_DATE,
    // admin actions
    ADD_CAR,
    DELETE_CAR,
    MODIFY_DATA,
};

enum
{
    CHANGE_PLATENUM,
    CHANGE_MODEL,
    CHANGE_YEAR,
    CHANGE_BRAND,
    CHANGE_COLOR,
    CHANGE_PRICEDAY,
    CHANGE_DATE,
    RETURN_TO_MENU
};

void printCars(Car *cars, int carsCount)
{
    cout << "Cars: (count: " << carsCount << "):\n";
    for (int i = 0; i < carsCount; i++)
    {
        Car car = cars[i];
        cout << "plate number: " << car.plateNumber << ", brand: " << car.brand
             << ", model: " << car.model << ", year: " << car.year
             << ", color: " << car.color
             << ", price per day: " << car.pricePerDay
             << (car.startDate == -1 ? " (available)" : " (rented)") << "\n";
    }
}

void printRentedCars(Car *cars, int carsCount)
{
    cout << "Rented Cars: (count: " << carsCount << "):\n";
    for (int i = 0; i < carsCount; i++)
    {
        Car car = cars[i];
        cout << "plate number: " << car.plateNumber << ", brand: " << car.brand
             << ", model: " << car.model << ", year: " << car.year
             << ", color: " << car.color
             << ", price per day: " << car.pricePerDay << "\n";
    }
}

void modifyDate(Car *car)
{
    time_t startDate = 0;
    time_t endDate = 0;

    time_t today = TimeOfTodayStart();

    do
    {
        if ((startDate = inputTime(true)) < today)
            cout << "start date should be greater than current date\n";
        else
            break;
    } while (true);

    do
    {
        if ((endDate = inputTime(false)) <= startDate)
            cout << "end date should be greater than the start date\n";
        else
            break;
    } while (true);

    car->startDate = startDate;
    car->endDate = endDate;
}

bool modifyCar(Car *car, Car *car1)
{
    int choice;

    bool hasChange = false;

    while (true)
    {
        cout << "Enter your choice: \n"
             << CHANGE_PLATENUM << ". change the plate number\n"
             << CHANGE_MODEL << ". change the model\n"
             << CHANGE_YEAR << ". change the year\n"
             << CHANGE_BRAND << ". change the brand\n"
             << CHANGE_COLOR << ". change the color\n"
             << CHANGE_PRICEDAY << ". change the price per day\n"
             << CHANGE_DATE << ". change the date\n"
             << RETURN_TO_MENU << ". stop modification\n";

        cout << "> ";
        cin >> choice;

        switch (choice)
        {
        case CHANGE_PLATENUM:
            cout << "Enter the new plate number: ";
            cin.ignore();
            cin >> car->plateNumber;
            car1->plateNumber = car->plateNumber;
            hasChange = true;
            break;

        case CHANGE_MODEL:
            cout << "Enter the new model: ";
            cin.ignore();
            cin >> car->model;
            car1->model = car->model;
            hasChange = true;
            break;

        case CHANGE_YEAR:
            cout << "Enter the new year: ";
            cin >> car->year;
            car1->year = car->year;
            hasChange = true;
            break;

        case CHANGE_BRAND:
            cout << "Enter the new brand: ";
            cin.ignore();
            cin >> car->brand;
            car1->brand = car->brand;
            hasChange = true;
            break;

        case CHANGE_COLOR:
            cout << "Enter the new color: ";
            cin.ignore();
            cin >> car->color;
            car1->color = car->color;
            hasChange = true;
            break;

        case CHANGE_PRICEDAY:
            cout << "Enter the new price per day: ";
            cin >> car->pricePerDay;
            car1->pricePerDay = car->pricePerDay;
            hasChange = true;
            break;

        case CHANGE_DATE:
            cout << "Enter the new date: ";
            cin.ignore();
            modifyDate(car);
            car1->startDate = car->startDate;
            car1->endDate = car->endDate;
            hasChange = true;
            break;

        case RETURN_TO_MENU:
            return hasChange;

        default:
            cout << "invalid choice!\n";
        }
    }
    return hasChange;
}

void addCarToFile(Car car);

void addCar(Car *&cars, int &carsCount)
{
    Car car = inputCar();
    cars_add(cars, carsCount, car);
    addCarToFile(car);
    cout << "added a new car to the available cars for rent.\n";
}

void loadCarsCSV(Car *&cars, int &size)
{
    try
    {
        io::CSVReader<6> in("cars.csv");
        in.read_header(io::ignore_extra_column, "plateNum", "Brand", "Year", "Model", "price_Day", "Color");
        Car car = {};
        while (in.read_row(car.plateNumber, car.brand, car.year, car.model, car.pricePerDay, car.color))
        {
            car.startDate = -1;
            car.endDate = -1;
            cars_add(cars, size, car);
            car = {};
        }
    }
    catch (exception &e)
    {
        // cars.csv does not exists
        cout << "error: " << e.what() << "\n";
    }
}

void loadClientsCSV(Client *&clients, int &size)
{
    try
    {
        io::CSVReader<7> in("clients.csv");
        in.read_header(io::ignore_extra_column, "ID", "fName", "lName", "Pass", "Email", "phoneNb", "admin");
        Client client = {};
        string admin = "";
        while (in.read_row(client.ID, client.firstName, client.lastName, client.password, client.email, client.phone, admin))
        {
            for (int i = 0; i < admin.length(); i++)
                admin[i] = tolower(admin[i]);
            client.admin = admin == "true"; // returns true if admin = true else returns false
            clients_add(clients, size, client);
            client = {};
            admin = "";
        }
    }
    catch (exception &e)
    {
        // clients.csv does not exists
        cout << "error: " << e.what() << "\n";
    }
}

void loadRentedCarsCSV(Client *clients, int clientsSize, Car *cars, int carsSize)
{
    try
    {
        io::CSVReader<4> in("rented-cars.csv");
        // ID,plateNumber,startDate,endDate
        in.read_header(io::ignore_extra_column, "ID", "plateNumber", "startDate", "endDate");
        int id;
        string plateNum, startDate, endDate;
        while (in.read_row(id, plateNum, startDate, endDate))
        {
            Client *client = clients_get(clients, clientsSize, id);
            if (client != NULL)
            {
                Car *car = cars_get(cars, carsSize, plateNum);
                if (car != NULL)
                {
                    car->startDate = StringToTime(startDate.c_str());
                    car->endDate = StringToTime(endDate.c_str());
                    cars_add(client->cars, client->nbReservation, *car);
                }
            }
        }
    }
    catch (exception &e)
    {
        // cars.csv does not exists
        cout << "error: " << e.what() << "\n";
    }
}

void rentCar(Car *cars, int carsCount, Client *client);

bool cancelRent(Client *client, Car *cars, int size);

bool deleteCar(Car *&cars, int &carsCount);

void writeCarRentInfo(ofstream &os, int id, Car car)
{
    os << "\n"
       << id << "," << car.plateNumber << ",";
    char date[20];
    TimeToString(date, 20, car.startDate);
    os << date << ",";
    TimeToString(date, 20, car.endDate);
    os << date;
}

void addRentCar(int id, Car car)
{
    ofstream os("rented-cars.csv", ios::app);
    writeCarRentInfo(os, id, car);
    os.close();
}

void writeCarsRentInfo(Client *clients, int size)
{
    ofstream os("rented-cars.csv");
    os << "ID,plateNumber,startDate,endDate";
    for (int i = 0; i < size; i++)
    {
        Client client = clients[i];
        for (int j = 0; j < client.nbReservation; j++)
        {
            Car car = client.cars[j];
            writeCarRentInfo(os, client.ID, car);
        }
    }
    os.close();
}

bool deleteCar(Car *&cars, int &carsCount)
{
    string plateNumber;
    cin.ignore();
    cout << "Enter the plate number of the car you want to delete: ";
    getline(cin, plateNumber);

    Car *car = cars_get(cars, carsCount, plateNumber);

    if (car == NULL)
        cout << "The car is not found in the rented cars.\n";
    else
    {
        cars_remove(cars, carsCount, plateNumber);
        return true;
    }
    return false;
}

void writeCarToFile(ofstream &os, Car car)
{
    os << "\n"
       << car.plateNumber << "," << car.brand << "," << car.year << ","
       << car.model << "," << car.pricePerDay << "," << car.color;
}

void writeCarsToFile(Car *cars, int size)
{
    ofstream os("cars.csv", ios::out);
    os << "plateNum,Brand,Year,Model,price_Day,Color";
    for (int i = 0; i < size; i++)
        writeCarToFile(os, cars[i]);
    os.close();
}

void addCarToFile(Car car)
{
    ofstream os("cars.csv", ios::app);
    writeCarToFile(os, car);
    os.close();
}

bool cancelRent(Client *client, Car *cars, int size)
{
    string plateNumber;
    cin.ignore();
    cout << "Enter the car plate number: ";
    getline(cin, plateNumber);

    Car *car = cars_get(client->cars, client->nbReservation, plateNumber);

    if (car == NULL)
        cout << "The car is not found in the rented cars.\n";
    else
    {
        Car *c = cars_get(cars, size, plateNumber);
        c->startDate = -1;
        c->endDate = -1;
        cars_remove(client->cars, client->nbReservation, plateNumber);
        return true;
    }

    return false;
}

void rentCar(Car *cars, int carsCount, Client *client)
{
    string plateNumber;
    cin.ignore();
    cout << "Enter the car plate number: ";
    getline(cin, plateNumber);

    Car *car = cars_get(cars, carsCount, plateNumber);

    if (car == NULL)
    {
        cout << "A car with such plate number does not exist.\n";
        return;
    }

    if (car->startDate != -1)
        cout << "The car is already rented, choose another one.\n";
    else
    {
        cout << "set the car rental date:\n";
        modifyDate(car);

        addRentCar(client->ID, *car);
        cars_add(client->cars, client->nbReservation, *car);
    }
}

void writePDF(Client *clients, int size);

void freeArrays(Car *&cars, int &carsCount, Client *&clients, int &clientsCount)
{
    if (carsCount != 0)
        delete[] cars;

    for (int i = 0; i < clientsCount; i++)
        if (clients[i].nbReservation > 0)
            delete[] clients[i].cars;

    if (clientsCount != 0)
        delete[] clients;
}

int main()
{
    Car *cars = NULL;
    int carsCount = 0;

    Client *clients = NULL;
    int clientsCount = 0;

    loadCarsCSV(cars, carsCount);
    loadClientsCSV(clients, clientsCount);
    loadRentedCarsCSV(clients, clientsCount, cars, carsCount);

    int choice;

    do
    {
        cout << "Welcome to the Car Rental System\n"
                "Please select an option: \n"
                "1. sign up\n"
                "2. sign in\n"
                "3. exit\n"
                "your choice: ";

        cin >> choice;
    } while (choice < 0 || choice > 3);

    if (choice == 3)
    {
        writePDF(clients, clientsCount);
        freeArrays(cars, carsCount, clients, clientsCount);
        return 0;
    }

    Client *client = NULL;

    if (choice == 1)
    {
        cin.ignore();
        Client c = inputClient(clients, clientsCount);
        c.ID = client_getNewID(clients, clientsCount);
        clients_add(clients, clientsCount, c);

        addClientToFile(c);

        cout << "Your client ID is " << c.ID << ", use it for logging in.\n";
    }

    do
    {
        int id;
        cout << "Enter your account ID: ";
        cin >> id;

        cout << "Enter your password: ";
        string password;
        cin.ignore();
        getline(cin, password);

        client = clients_get(clients, clientsCount, id);
        if (client == NULL)
        {
            cout << "invalid credentials, try again.\n";
            continue;
        }

        SHA256 sha = SHA256();          // SHA = secure hashing algorithm
        sha.update(password);           // update do the hashing
        uint8_t *digest = sha.digest(); // digest returns the hash
        password = SHA256::toString(
            digest); // transform it to hex string, 0 -> 00, 9 -> 09, 255 -> FF
        delete[] digest;

        if (password != client->password)
        {
            cout << "invalid credentials, try again.\n";
            continue;
        }

        break;

    } while (true);

    bool exit = false;
    do
    {
        cout << "Select an option:\n"
             << EXIT << ". exit\n"
             << LIST_CARS << ". list cars\n"
             << LIST_RENTED_CARS << ". list rented cars\n"
             << RENT_CAR << ". rent a car\n"
             << CANCEL_RENT << ". cancel a car rent\n"
             << MODIFY_DATE << ". mofify the rental date\n";

        if (client->admin)
        {
            cout << ADD_CAR << ". add a car\n"
                 << DELETE_CAR << ". remove a car\n"
                 << MODIFY_DATA << ". modify the data of a car\n";
        }

        cout << "> ";
        cin >> choice;

        if (client->admin)
        {
            bool handled = true;
            switch (choice)
            {
            case ADD_CAR:
            {
                cin.ignore();
                addCar(cars, carsCount);
            }
            break;

            case DELETE_CAR:
            {
                if (deleteCar(cars, carsCount))
                    writeCarsToFile(cars, carsCount);
            }
            break;

            case MODIFY_DATA:
            {
                string plateNumber;
                int ID;
                cout << "Enter the plate number of the car: ";
                cin.ignore();
                getline(cin, plateNumber);
                Car *car = cars_get(cars, carsCount, plateNumber);
                Car *car1 =
                    cars_get_client(clients, clientsCount, plateNumber, ID);
                if (car == NULL)
                {
                    cout << "The car is not found in the rented cars.\n";
                    continue;
                }
                if (modifyCar(car, car1))
                {
                    writeCarsRentInfo(clients, clientsCount);
                    writeCarsToFile(cars, carsCount);
                }
            }
            break;

            default:
                handled = false;
                break;
            }

            if (handled)
                continue;
        }

        switch (choice)
        {
        case EXIT:
            exit = true;
            break;

        case LIST_CARS:
            printCars(cars, carsCount);
            break;

        case LIST_RENTED_CARS:
            printRentedCars(client->cars, client->nbReservation);
            break;

        case RENT_CAR:
            rentCar(cars, carsCount, client);
            break;

        case CANCEL_RENT:
            if (cancelRent(client, cars, carsCount))
                writeCarsRentInfo(clients, clientsCount);
            break;

        case MODIFY_DATE:
        {
            string plateNumber;
            cin.ignore();
            cout << "Enter the car plate number: ";
            getline(cin, plateNumber);

            Car *car =
                cars_get(client->cars, client->nbReservation, plateNumber);

            if (car == NULL)
                cout << "The car is not found in the rented cars.\n";
            else
            {
                modifyDate(car);
                writeCarsRentInfo(clients, clientsCount);
            }
        }
        break;

        default:
            cout << "invalid choice!\n";
            break;
        }
    } while (!exit);

    writePDF(clients, clientsCount);

    freeArrays(cars, carsCount, clients, clientsCount);

    return 0;
}

struct RentedCar
{
    Client *client;
    Car *car;
};

void addRentedCar(RentedCar *&cars, int &size, RentedCar rentedCar)
{
    RentedCar *newRentedCars = new RentedCar[size + 1];
    for (int i = 0; i < size; i++)
        newRentedCars[i] = cars[i];
    newRentedCars[size] = rentedCar;

    if (size != 0)
        delete[] cars;

    cars = newRentedCars;
    size += 1;
}

void writePDF(Client *clients, int size)
{
    RentedCar *rentedCars = NULL;
    int rentedCarsCount = 0;

    for (int i = 0; i < size; i++)
    {
        Client *client = &clients[i];
        for (int j = 0; j < client->nbReservation; j++)
        {
            RentedCar car;
            car.car = &client->cars[j];
            car.client = client;
            addRentedCar(rentedCars, rentedCarsCount, car);
        }
    }

    for (int i = 0; i < rentedCarsCount; i++)
    {
        for (int j = 0; j < rentedCarsCount; j++)
        {
            if (rentedCars[i].car->pricePerDay < rentedCars[j].car->pricePerDay)
            {
                RentedCar temp = rentedCars[i];
                rentedCars[i] = rentedCars[j];
                rentedCars[j] = temp;
            }
        }
    }

    struct pdf_info info = {.creator = "Marven Eid",
                            .producer = "",
                            .title = "Car Rental System",
                            .author = "Marven Eid",
                            .subject = "Rented Cars",
                            .date = "Today"};

    const char *fontName = "Times-Roman";
    const float fontSize = 16;
    const float titleHeight =
        PDF_A4_HEIGHT - 50; // position height starts from down as 0
    const float lineHeight = 30;
    const float textWidth = 50;
    const char *title = "Rented Cars List";

    pdf_doc *pdf = pdf_create(PDF_A4_WIDTH, PDF_A4_HEIGHT, &info);
    pdf_set_font(pdf, fontName);
    pdf_object *page = pdf_append_page(pdf);

    float width;
    pdf_get_font_text_width(pdf, fontName, title, fontSize + 4, &width);

    float titleWidthStart = (PDF_A4_WIDTH) / 2 - width / 2;
    float titleWidthEnd = (PDF_A4_WIDTH) / 2 + width / 2;

    pdf_add_text(pdf, page, title, fontSize + 4, titleWidthStart, titleHeight,
                 PDF_BLACK);
    pdf_add_line(pdf, page, titleWidthStart, titleHeight - 5, titleWidthEnd,
                 titleHeight - 5, 1, PDF_BLACK); // l khat li tht l title

    float textHeight = titleHeight - lineHeight * 2;

    for (int i = 0; i < rentedCarsCount; i++)
    {
        float height;

        RentedCar rentedCar = rentedCars[i];
        Car *car = rentedCar.car;
        Client *client = rentedCar.client;
        ostringstream text;
        text.precision(2);

        char StatDate[20];
        char EndDate[20];
        TimeToString(StatDate, 20, car->startDate);
        TimeToString(EndDate, 20, car->endDate);

        text << "- brand " << car->brand << ", model " << car->model
             << ", year " << car->year << ", color " << car->color
             << " rented by " << client->firstName << " " << client->lastName
             << " (user id: " << client->ID << ")"
             << " from " << StatDate << " till " << EndDate << " for " << fixed
             << rentedCar.car->pricePerDay << "$";

        pdf_add_text_wrap(pdf, page, text.str().c_str(), fontSize, textWidth,
                          textHeight, 0, PDF_BLACK,
                          PDF_A4_WIDTH - textWidth * 2, PDF_ALIGN_JUSTIFY,
                          &height);

        textHeight -= height;
        textHeight -= lineHeight;

        if ((textHeight - lineHeight * 2) < 0)
        {
            page = pdf_append_page(pdf);
            textHeight = PDF_A4_HEIGHT - lineHeight * 2;
        }
    }

    pdf_save(pdf, "rented-cars-report.pdf");
    pdf_destroy(pdf);

    if (rentedCars != NULL)
        delete[] rentedCars;
}