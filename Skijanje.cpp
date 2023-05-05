/**
 * Napisati C++ program koji iz ulazne datoteke cita podatke o temperaturi u toku vikenda sa ski 
 * staza na Kopaoniku, Zlatiboru i Jahorini i odredjuje koji dani i na kojoj planini su idealni 
 * za skijanje a koji nisu. Neki idealni opseg temperature za skijanje je od -5 do 3 stepena.
 * 
 * Za svaku od planina postoji posebna datoteka cije ime se sastoji od imena planine i prosirenja
 * (ekstenzije) ".txt". U svakoj pojedinacnoj datoteci se u jednom redu nalaze podaci za jedan dan. 
 * Jedan red sadrzi redom ime_dana, datum, i potom izmerene temperature. Temperatura se meri na 
 * svakih sat vremena, pocevsi od 8 ujutru, do 8 uvece. Svi podaci su odvojeni razmakom.
 * 
 * Izgled jednog reda iz ulaznih datoteka "Kopaonik.txt" "Zlatibor.txt" "Jahorina.txt"
 * 
 *     subota 01.02. -15 -13 -10 -8 -3 0 -2 -3 2 2 -5 -7 -3
 * 
 * Treba za svaki dan pronaci najnizu i najvisu dnevnu temperaturu. Ukoliko minimalna i maksimalna
 * temperatura upadaju u navedeni opseg, treba informacije za taj dan ispisati u datoteku 
 * "idealno.txt", u suprotnom u datoteku "lose.txt".
 *
 * Ispis u izlaznu datoteku treba da prati format:
 *     <ime_planine> [<ime_dana> <datum>] <min. temp.> >> <maks. temp.>
 * 
 * Primer ispisa u bilo kojoj od izlaznih datoteka "idealno.txt", "lose.txt":
 * 
 *     Kopaonik [subota 01.02.] -15 >> 2
 *
 * Treba napraviti jednu nit koja ce samo citati podatke iz ulaznih datoteka, jednu nit koja ce 
 * samo pisati spremljene podatke u izlazne datoteke i 4 niti radnika koji ce na osnovu podataka iz
 * ulaznih datoteka generisati sve neophodno za ispis u izlazne datoteke.
*/
#include <mutex>
#include <thread>
#include <condition_variable>
#include <fstream>
#include <sstream>
#include <queue>
#include <iostream>
using namespace std;
/// subota 01.02. -15 -13 -10 -8 -3 0 -2 -3 2 2 -5 -7 -3
/// Kopaonik [subota 01.02.] -15 >> 2
/// Jahorina subota 01.03. -15 -13 -10 -8 -3 0 -2 -3 2 2 -5 -7 -3
class Planina {
private:
    string imePlanine;
    string dan;
    string datum;
    string temperature;
    int min, max;
public:
    Planina(){}
    Planina (string ip, string da, string dat, string t){
        imePlanine = ip;
        dan = da;
        datum = dat;
        temperature = t;
    }
    void findMinMax(){
        istringstream str(temperature);
        string temp = "";
        getline(str, temp, ' ');
        min = stoi(temp);
        max = stoi(temp);
        while(getline(str, temp, ' ')){
            if(stoi(temp) < min){
                min = stoi(temp);
            }
            if(stoi(temp) > max){
                max = stoi(temp);
            }
        }
        //cout<<min<<" "<<max<<endl;
        
    }
    int getMin(){
        return min;
    }
    int getMax(){
        return max;
    }
    /// Kopaonik [subota 01.02.] -15 >> 2
    friend ostream& operator<<(ostream& out, const Planina& p){
        out<<p.imePlanine<<" ["<<p.dan<<" "<<p.datum<<"] "<<p.min<<" >> "<<p.max<<endl;
        return out;
    }

};

template<typename T>
class InputData {
private:
    mutex m;
    condition_variable spremni;
    bool kraj;
    queue<T> kolekcija;
public:
    InputData(): kraj(false) {}
    void add_data(T data_element) {
        unique_lock<mutex> lock(m);
        kolekcija.push(data_element);
    }
    bool remove_data(T &data_element) {
        unique_lock<mutex> lock(m);
        while(kolekcija.empty() && !kraj){
            spremni.wait(lock);
        }
        if(kolekcija.empty() && kraj){
            return false;
        }
        data_element = kolekcija.front();
        kolekcija.pop();
        return true;
    }
    void objaviKraj(){
        unique_lock<mutex> lock(m);
        kraj = true;
        spremni.notify_all();
    }

};

template<typename T>
class OutputData {
private:
    mutex m;
    condition_variable spremni;
    queue<T> kolekcija;
    bool kraj;
    int workers_num;
public:
    OutputData(): kraj(false), workers_num(0) {}
    void add_data(T data_element) {
        unique_lock<mutex> lock(m);
        kolekcija.push(data_element);
    }
    bool remove_data(T &data_element) {
        unique_lock<mutex> lock(m);
        while(kolekcija.empty() && !kraj){
            spremni.wait(lock);
        }
        if(kolekcija.empty() && kraj){
            return false;
        }
        data_element = kolekcija.front();
        kolekcija.pop();
        return true;
    }
    void prijaviRadnika(){
        unique_lock<mutex> lock(m);
        workers_num++;
    }
    void odjaviRadnika(){
        unique_lock<mutex> lock(m);
        workers_num--;
        if(workers_num == 0){
            kraj = true;
            spremni.notify_one();
        }
    }
};
/// subota 01.02. -15 -13 -10 -8 -3 0 -2 -3 2 2 -5 -7 -3
/// Kopaonik [subota 01.02.] -15 >> 2
void reader(vector<string> input_filenames, InputData<string>& raw_data) {
    ifstream kopaonik_dat(input_filenames[0]);
    ifstream zlatibor_dat(input_filenames[1]);
    ifstream jahorina_dat(input_filenames[2]);
    string line1;
    if(kopaonik_dat.is_open()){
        istringstream str(input_filenames[0]);   //u stream stavljamo Kopaonik.txt
        string planina1;
        getline(str, planina1, '.');    //ovo da bi zaobisli .txt, znaci citamo do tacke
        //string planina1 = "Kopaonik ";
        while(getline(kopaonik_dat, line1)){
            string help1 = "";
            help1 = planina1 + " " + line1;
            //cout<<help1<<endl;
            raw_data.add_data(help1);
        }
    }
    kopaonik_dat.close();
    string line2;
    if(zlatibor_dat.is_open()){
        string planina2 = "Zlatibor ";
        while(getline(zlatibor_dat, line2)){
            string help2 = "";
            help2 = planina2 + line2;
            raw_data.add_data(help2);
            //cout<<help2<<endl;
        }
    }
    zlatibor_dat.close();
    string line3;
    if(jahorina_dat.is_open()){
        string planina3 = "Jahorina ";
        while(getline(jahorina_dat, line3)){
            string help3 = "";
            help3 = planina3 + line3;
            raw_data.add_data(help3);
            //cout<<help3<<endl;
        }
    }
    jahorina_dat.close();
    raw_data.objaviKraj();
}
/// Jahorina subota 01.03. -15 -13 -10 -8 -3 0 -2 -3 2 2 -5 -7 -3
void proccessing_data(InputData<string>& raw_data, OutputData<Planina>& proccessed_data){
    proccessed_data.prijaviRadnika();
    string imePlanine, dan, datum, temperature;
    string line;
    while(raw_data.remove_data(line)){
        istringstream str(line);
        getline(str, imePlanine, ' ');
        getline(str, dan, ' ');
        getline(str, datum, ' ');
        getline(str, temperature);
        Planina p(imePlanine, dan, datum, temperature);
        p.findMinMax();
        cout<<p;
        proccessed_data.add_data(p);
    }
    proccessed_data.odjaviRadnika();
}

void writer(OutputData<Planina>& proccessed_data) {
    ofstream idealno("idealno.txt");
    ofstream nakazno("nakazno.txt");
    Planina p;
    while(proccessed_data.remove_data(p)){
        if(p.getMax() <= 3 && p.getMin() >= -5){
            idealno<<p;
        }else{
            nakazno<<p;
        }
    }
    nakazno.close();
    idealno.close();
}

int main() {
    InputData<string> raw_data;
    OutputData<Planina> proccessed_data;
    vector<string> input_filenames = {"Kopaonik.txt", "Zlatibor.txt", "Jahorina.txt"};

    thread th_reader(reader, input_filenames, ref(raw_data));
    thread th_writer(writer, ref(proccessed_data));
    thread th_workers[4];

    for(auto &th: th_workers){
        th = thread(proccessing_data, ref(raw_data), ref(proccessed_data));
    }

    th_reader.join();
    for(auto &th: th_workers) {
        th.join();
    }
    th_writer.join();
    
    return 0;
}
