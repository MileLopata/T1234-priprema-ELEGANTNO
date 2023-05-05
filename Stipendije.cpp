/**
 * Napisati C++ program koji cita podatke o studentima iz ulazne datoteke i potom 
 * za svakog studenta racuna prosek
 * 
 * U ulaznoj datoteci "studenti.csv" se u svakom redu nalaze informacije o studentu: 
 *     Ime,Prezime,Broj indeksa,Ocene 
 * Pritom su ocene odvojene medjusobno zarezima.
 * 
 * Prilikom obrade podataka o studentima, mora se proveriti format indeksa da li je validan. Ako nije, zanemariti taj unos.
 * Format indeksa je:
 *     [[:alpha:]][[:alnum:]]{1,2}\\s[[:digit:]]{1,3}\/[[:digit:]]{4}
 * 
 * U tri izlazne datoteke rasporediti studente u zavisnosti od proseka.
 * Ukoliko je prosek > 9.00 potrebno je upisati studenta u datoteku "kandidati_stipendija.csv".
 * Ukoliko je prosek > 8.00 i prosek <= 9.00 potrebno je upisati studenta u datoteku "kandidati_kredit.csv".
 * U ostalim slucajevima upisati studenta u datoteku "ostali.csv".
 * Format u izlaznoj datoteci treba da odgovara sledecem: Ime,Prezime,Broj_indeksa,prosek
 * 
 * Treba napraviti jednu nit koja ce samo citati redove ulazne podatke, jednu nit 
 * koja ce samo pisati gotove podatke u izlazne datoteke i 10 niti radnika koji ce na osnovu redova
 * iz ulazne datoteke generisati sve neophodno za ispis u izlaznu datoteku.
*/
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <random>
#include <chrono>
#include <fstream>
#include <regex>
#include <vector>
using namespace std;
using namespace chrono;

class Student {
private:
    string ime, prezime, indeks, ocene;
    double prosek;
public:
    Student(){}
    Student(string i, string p, string ind, string o){
        ime = i;
        prezime = p;
        indeks = ind;
        ocene = o;
    }
    double getProsek(){
        return prosek;
    }
    void izracunaj(){
        istringstream str(ocene);
        double sum = 0;
        double count = 0;
        string ocena;
        while(getline(str, ocena, ',')){
            sum += stod(ocena);
            count++;
        }
        prosek = sum / count;
    }
    friend ostream& operator<<(ostream& out, Student& s){
        out<<s.ime<<","<<s.prezime<<","<<s.indeks<<","<<s.prosek<<endl;;
        return out;
    }
};


template<typename T>
class InputData {    //InputData
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
    int data_producers_num;
public:
    OutputData(): kraj(false), data_producers_num(0) {}
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
        data_producers_num++;
    }
    void odjaviRadnika(){
        unique_lock<mutex> lock(m);
        data_producers_num--;
        if(data_producers_num == 0){
            kraj = true;
            spremni.notify_one();
        }
    }
};

void reader(string input_file_name, InputData<string>& raw_data) {  
    ifstream ulazna_dat(input_file_name);
    string line;
    if(ulazna_dat.is_open()){
        while(getline(ulazna_dat, line)){
            raw_data.add_data(line);
            //cout<<line<<endl;
        }
    }
    ulazna_dat.close();
    raw_data.objaviKraj();
}

void proccessing_data(InputData<string>& raw_data, OutputData<Student>& proccessed_data){
    proccessed_data.prijaviRadnika();
    string ime, prezime, indeks, ocene;
    string line;
    regex r("[[:alpha:]][[:alnum:]]{1,2}\\s[[:digit:]]{1,3}\/[[:digit:]]{4}");
    while(raw_data.remove_data(line)){
        istringstream str(line);
        getline(str, ime, ',');
        getline(str, prezime, ',');
        getline(str, indeks, ',');
        getline(str, ocene);
        if(regex_match(indeks, r)){
            Student s(ime, prezime, indeks, ocene);
            s.izracunaj();
            cout<<s;
            proccessed_data.add_data(s);
        }
    }
    proccessed_data.odjaviRadnika();
}

void writer(OutputData<Student>& proccessed_data) {
    ofstream stipendije("elegantneStipendije.csv");
    ofstream krediti("elegantniKrediti.csv");
    ofstream ostali("elegantniOstali.csv");
    Student s;
    while(proccessed_data.remove_data(s)){
        if(s.getProsek() > 9.00){
            stipendije<<s;
        }else if(s.getProsek() < 9.00 && s.getProsek() > 8.00){
            krediti<<s;
        }else{
            ostali<<s;
        }
    }
    stipendije.close();
    krediti.close();
    ostali.close();
}
int main() {
    InputData<string> raw_data;
    OutputData<Student> proccessed_data;

    thread th_reader(reader, "studenti.csv", ref(raw_data));
    thread th_writer(writer, ref(proccessed_data));
    thread th_workers[10];

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
