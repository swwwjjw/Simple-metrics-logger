#include "MetricsVK.h"

//Базовый класс для метрик
class Metric {
public: 
	virtual std::string getName() const = 0;
	virtual std::string getValueAsString() const = 0;
	virtual void reset() = 0;
	virtual ~Metric() = default;
};

//Класс для конкретных метрик по типу
template<typename T>
class ConcreteMetric : public Metric {
	std::string _name;
	std::atomic<T> _value;
public:
	ConcreteMetric(const std::string& metricName) : _name(metricName), _value(0) {}
	void add(T new_value) {
		_value += new_value;
	}
	void set(T new_value) {
		_value = new_value;
	}
	std::string getName() const override {
		return _name;
	}
	std::string getValueAsString() const override {
		std::ostringstream oss;
		if (std::is_floating_point_v<T>) {
			oss << std::fixed << std::setprecision(2) << _value.load();
		}
		else {
			oss << _value.load();
		}
		return oss.str();
	}
	void reset() override {
		_value = 0;
	}
};

//Менеджер (создание/изменение метрик, логирование в файл)
class Manager {
	std::unordered_map<std::string, std::unique_ptr<Metric>> _metrics;
	std::fstream _file;
public:
	Manager(const std::string& name) {
		_file.open(name, std::ios::app);
		if (!_file.is_open()) {
			throw std::runtime_error("Cannot open file!");
		}
	}
	~Manager() {
		_file.close();
	}
	template<typename T>
	void createMetric(const std::string& name) {
		_metrics[name] = std::make_unique<ConcreteMetric<T>>(name);
	}
	void deleteMetric(const std::string& name) {
		_metrics.erase(name);
	}
	template<typename T>
	void addValue(const std::string& name, T value) {
		auto tm = _metrics.find(name);
		if (tm != _metrics.end()) {
			auto* metric = dynamic_cast<ConcreteMetric<T>*>(tm->second.get());
			if (metric) {
				metric->add(value);
			}
		}
	}
	template<typename T>
	void setValue(const std::string& name, T value) {
		auto tm = _metrics.find(name);
		if (tm != _metrics.end()) {
			auto* metric = dynamic_cast<ConcreteMetric<T>*>(tm->second.get());
			if (metric) {
				metric->set(value);
			}
		}
	}
	void logMetrics() {
		auto now = std::chrono::system_clock::now();
		auto time = std::chrono::system_clock::to_time_t(now);
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
		std::stringstream timestamp;
		timestamp << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S.") << std::setfill('0') << std::setw(3) << ms.count() << " ";
		_file << timestamp.str();
		for (const auto& [name, metric] : _metrics) {
			_file << '"' << name << '"' << " " << metric->getValueAsString() << " ";
			metric->reset();
		}
		_file << std::endl;
	}
};

int main() {
	try {
		//Имитируем работу, инициализируем менеджер
		Manager manager("metrics.txt");

		//Инициализируем метрики
		manager.createMetric<double>("CPU");
		manager.createMetric<int>("HTTP requests RPS");

		//Добавляем значения метрик
		manager.setValue<double>("CPU", 0.97);
		manager.setValue<int>("HTTP requests RPS", 42);

		//Логируем метрики
		manager.logMetrics();

		//Добавляем значения метрик
		manager.setValue<double>("CPU", 1.12);
		manager.setValue<int>("HTTP requests RPS", 30);

		//Логируем метрики
		manager.logMetrics();

		//Инициализируем метрику
		manager.createMetric<int>("CPU temperature");

		for (int i = 0; i < 3; i++) {
			//Добавляем значения метрик
			manager.setValue<double>("CPU", 0.5 + i * 0.1);
			manager.addValue<int>("HTTP requests RPS", 10 + i * 2);
			manager.addValue<int>("CPU temperature", 50 + i * 2);
			//Логируем метрики
			manager.logMetrics();
		}

		//Удаляем ненужные метрики
		manager.deleteMetric("CPU");
		manager.deleteMetric("CPU temperature");

		//Добавляем значение метрике после обнуления
		manager.addValue<int>("HTTP requests RPS", 1);

		//Логируем метрику
		manager.logMetrics();
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
