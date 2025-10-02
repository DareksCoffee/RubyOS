import unittest
import sys
from colorama import init, Fore, Back, Style
import time

init(autoreset=True)

class FancyTestResult(unittest.TextTestResult):
    def startTest(self, test):
        super().startTest(test)
        self.stream.write(f"{Fore.CYAN}Running: {test._testMethodName}... {Style.RESET_ALL}")
        self.stream.flush()

    def addSuccess(self, test):
        super().addSuccess(test)
        self.stream.write(f"{Fore.GREEN}✓ PASSED{Style.RESET_ALL}\n")

    def addError(self, test, err):
        super().addError(test, err)
        self.stream.write(f"{Fore.RED}✗ ERROR{Style.RESET_ALL}\n")

    def addFailure(self, test, err):
        super().addFailure(test, err)
        self.stream.write(f"{Fore.RED}✗ FAILED{Style.RESET_ALL}\n")

    def addSkip(self, test, reason):
        super().addSkip(test, reason)
        self.stream.write(f"{Fore.YELLOW}⚠ SKIPPED{Style.RESET_ALL}\n")

class FancyTestRunner(unittest.TextTestRunner):
    def __init__(self, *args, **kwargs):
        kwargs['resultclass'] = FancyTestResult
        super().__init__(*args, **kwargs)

    def run(self, test):
        self.stream.write(f"{Fore.MAGENTA}{Back.WHITE}🚀 Starting RubyOS Test Suite{Style.RESET_ALL}\n")
        self.stream.write(f"{Fore.BLUE}{'='*50}{Style.RESET_ALL}\n")
        start_time = time.time()
        result = super().run(test)
        end_time = time.time()
        duration = end_time - start_time

        self.stream.write(f"{Fore.BLUE}{'='*50}{Style.RESET_ALL}\n")
        self.stream.write(f"{Fore.MAGENTA}Test Summary:{Style.RESET_ALL}\n")
        self.stream.write(f"Ran {result.testsRun} tests in {duration:.3f}s\n")

        if result.wasSuccessful():
            self.stream.write(f"{Fore.GREEN}🎉 All tests passed!{Style.RESET_ALL}\n")
        else:
            self.stream.write(f"{Fore.RED}❌ Some tests failed.{Style.RESET_ALL}\n")
            if result.errors:
                self.stream.write(f"{Fore.RED}Errors: {len(result.errors)}{Style.RESET_ALL}\n")
            if result.failures:
                self.stream.write(f"{Fore.RED}Failures: {len(result.failures)}{Style.RESET_ALL}\n")

        return result

if __name__ == '__main__':
    loader = unittest.TestLoader()
    suite = loader.discover(start_dir='.', pattern='test_*.py')

    runner = FancyTestRunner(verbosity=0, stream=sys.stdout)
    result = runner.run(suite)
    sys.exit(0 if result.wasSuccessful() else 1)